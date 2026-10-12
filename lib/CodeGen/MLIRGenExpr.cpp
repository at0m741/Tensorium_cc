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

  if (auto *binary = dynamic_cast<const BinaryExpr *>(&expr))
    return emitBinary(*binary);

  diag.error(expr.loc, "MLIR generation does not support this expression yet");
  return {};
}
