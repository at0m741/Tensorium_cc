#include "cc1/CodeGen/MLIRGen.hpp"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "llvm/ADT/SmallVector.h"

mlir::Value MLIRGen::emitExpr(const Expr &expr) {
  if (auto *literal = dynamic_cast<const IntLitExpr *>(&expr)) {
    if (literal->type && literal->type->isIntegral()) {
      mlir::Type type = lowerType(*literal->type, literal->loc);
      if (!type)
        return {};

      auto intType = mlir::dyn_cast<mlir::IntegerType>(type);
      if (!intType) {
        diag.error(literal->loc,
                   "integer literal lowered to a non-integer MLIR type");
        return {};
      }

      return mlir::arith::ConstantIntOp::create(
                 builder, builder.getUnknownLoc(),
                 static_cast<int64_t>(literal->val), intType.getWidth())
          .getResult();
    }
  }

  if (auto *ref = dynamic_cast<const IdentExpr *>(&expr)) {
    auto slot = storage.find(ref->decl);
    if (slot != storage.end())
      return emitLoad(slot->second);
    auto it = values.find(ref->decl);
    if (it == values.end()) {
      diag.error(ref->loc, "unresolved declaration in MLIR generation");
      return {};
    }
    return it->second;
  }

  if (auto *cast = dynamic_cast<const ImplicitCastExpr *>(&expr)) {
    if (!cast->operand || !cast->operand->type || !cast->type) {
      diag.error(cast->loc, "untyped implicit conversion in MLIR generation");
      return {};
    }
    if (cast->kind == ImplicitCastKind::LValueToRValue)
      return emitExpr(*cast->operand);

    if (cast->kind == ImplicitCastKind::Arithmetic &&
        cast->operand->type->isIntegral() && cast->type->isIntegral()) {
      mlir::Value value = emitExpr(*cast->operand);
      if (!value)
        return {};
      auto from = mlir::dyn_cast<mlir::IntegerType>(value.getType());
      mlir::Type destination = lowerType(*cast->type, cast->loc);
      if (!destination)
        return {};
      auto to = mlir::dyn_cast<mlir::IntegerType>(destination);
      if (!from || !to) {
        diag.error(cast->loc, "integer conversion requires integer MLIR types");
        return {};
      }
      auto loc = builder.getUnknownLoc();
      if (cast->type->kind == Type::BOOL) {
        auto zero = mlir::arith::ConstantIntOp::create(builder, loc, from, 0);
        return mlir::arith::CmpIOp::create(builder, loc,
                                           mlir::arith::CmpIPredicate::ne,
                                           value, zero.getResult())
            .getResult();
      }
      if (from == to)
        return value;
      if (from.getWidth() > to.getWidth())
        return mlir::arith::TruncIOp::create(builder, loc, to, value)
            .getResult();
      if (cast->operand->type->isSigned())
        return mlir::arith::ExtSIOp::create(builder, loc, to, value)
            .getResult();
      return mlir::arith::ExtUIOp::create(builder, loc, to, value).getResult();
    }
    diag.error(cast->loc,
               "MLIR generation does not support this implicit conversion yet");
    return {};
  }

  if (auto *call = dynamic_cast<const CallExpr *>(&expr)) {
    mlir::Value result;
    if (mlir::failed(emitCall(*call, &result)))
      return {};
    return result;
  }

  if (auto *binary = dynamic_cast<const BinaryExpr *>(&expr)) {
    if (binary->op == BinaryOp::ASSIGN) {
      mlir::Value address = emitLValue(*binary->lhs);
      if (!address)
        return {};
      mlir::Value value = emitExpr(*binary->rhs);
      if (!value)
        return {};
      auto type = mlir::cast<mlir::MemRefType>(address.getType());
      if (value.getType() != type.getElementType()) {
        diag.error(binary->loc, "MLIR assignment types do not match the AST");
        return {};
      }
      emitStore(value, address);
      return value;
    }
    if (binary->op >= BinaryOp::EQ && binary->op <= BinaryOp::GEQ) {
      if (!binary->lhs || !binary->rhs || !binary->lhs->type ||
          !binary->rhs->type || !binary->lhs->type->isIntegral() ||
          !binary->rhs->type->isIntegral() || !binary->type ||
          binary->type->kind != Type::INT) {
        diag.error(binary->loc,
                   "MLIR comparisons currently require integer operands");
        return {};
      }

      mlir::Value lhs = emitExpr(*binary->lhs);
      if (!lhs)
        return {};

      mlir::Value rhs = emitExpr(*binary->rhs);
      if (!rhs)
        return {};

      if (!mlir::isa<mlir::IntegerType>(lhs.getType()) ||
          lhs.getType() != rhs.getType()) {
        diag.error(binary->loc,
                   "MLIR comparison operand types do not match the AST");
        return {};
      }

      const bool isSigned = binary->lhs->type->isSigned();
      using Predicate = mlir::arith::CmpIPredicate;
      Predicate predicate = Predicate::eq;

      switch (binary->op) {
      case BinaryOp::EQ:
        predicate = Predicate::eq;
        break;
      case BinaryOp::NEQ:
        predicate = Predicate::ne;
        break;
      case BinaryOp::LT:
        predicate = isSigned ? Predicate::slt : Predicate::ult;
        break;
      case BinaryOp::GT:
        predicate = isSigned ? Predicate::sgt : Predicate::ugt;
        break;
      case BinaryOp::LEQ:
        predicate = isSigned ? Predicate::sle : Predicate::ule;
        break;
      case BinaryOp::GEQ:
        predicate = isSigned ? Predicate::sge : Predicate::uge;
        break;
      default:
        break;
      }

      mlir::Type resultType = lowerType(*binary->type, binary->loc);
      if (!resultType)
        return {};

      auto loc = builder.getUnknownLoc();
      mlir::Value comparison =
          mlir::arith::CmpIOp::create(builder, loc, predicate, lhs, rhs)
              .getResult();

      return mlir::arith::ExtUIOp::create(builder, loc, resultType, comparison)
          .getResult();
    }
    if (binary->op != BinaryOp::ADD && binary->op != BinaryOp::SUB &&
        binary->op != BinaryOp::MUL) {
      diag.error(binary->loc,
                 "MLIR generation does not support this binary operator yet");
      return {};
    }
    if (!binary->type || !binary->type->isIntegral() || !binary->lhs ||
        !binary->rhs) {
      diag.error(binary->loc,
                 "MLIR binary arithmetic currently requires integer operands");
      return {};
    }
    mlir::Value lhs = emitExpr(*binary->lhs);
    if (!lhs)
      return {};
    mlir::Value rhs = emitExpr(*binary->rhs);
    if (!rhs)
      return {};
    mlir::Type resultType = lowerType(*binary->type, binary->loc);
    if (!resultType || !mlir::isa<mlir::IntegerType>(lhs.getType()) ||
        lhs.getType() != rhs.getType() || lhs.getType() != resultType) {
      diag.error(binary->loc, "MLIR binary operand types do not match the AST");
      return {};
    }
    auto loc = builder.getUnknownLoc();
    switch (binary->op) {
    case BinaryOp::ADD:
      return mlir::arith::AddIOp::create(builder, loc, lhs, rhs).getResult();
    case BinaryOp::SUB:
      return mlir::arith::SubIOp::create(builder, loc, lhs, rhs).getResult();
    case BinaryOp::MUL:
      return mlir::arith::MulIOp::create(builder, loc, lhs, rhs).getResult();
    default:
      break;
    }
  }
  diag.error(expr.loc, "MLIR generation does not support this expression yet");
  return {};
}

mlir::Value MLIRGen::emitCondition(const Expr &expr) {
  mlir::Value value = emitExpr(expr);
  if (!value)
    return {};

  auto type = mlir::dyn_cast<mlir::IntegerType>(value.getType());
  if (!type) {
    diag.error(expr.loc, "MLIR conditions currently require an integer value");
    return {};
  }

  if (type.getWidth() == 1)
    return value;

  auto loc = builder.getUnknownLoc();

  auto zero =
      mlir::arith::ConstantIntOp::create(builder, loc, 0, type.getWidth());
  return mlir::arith::CmpIOp::create(builder, loc,
                                     mlir::arith::CmpIPredicate::ne, value,
                                     zero.getResult())
      .getResult();
}
