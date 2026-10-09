#include "MLIRGen.hpp"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"

mlir::Value MLIRGen::emitBinary(const BinaryExpr &binary) {
  if (binary.op == BinaryOp::ASSIGN) {
    auto *ref = dynamic_cast<const IdentExpr *>(binary.lhs);
    if (!ref || !ref->decl) {
      diag.error(binary.loc, "MLIR assignment requires a scalar variable");
      return {};
    }
    auto slot = storage.find(ref->decl);
    if (slot == storage.end()) {
      auto value = values.find(ref->decl);
      if (value == values.end()) {
        diag.error(ref->loc, "unresolved assignment target in MLIR generation");
        return {};
      }
      auto type = mlir::MemRefType::get({}, value->second.getType());
      auto address =
          builder.create<mlir::memref::AllocaOp>(builder.getUnknownLoc(), type)
              .getResult();
      builder.create<mlir::memref::StoreOp>(
          builder.getUnknownLoc(), value->second, address, mlir::ValueRange{});
      storage[ref->decl] = address;
    }
    mlir::Value value = emitExpr(*binary.rhs);
    if (!value)
      return {};
    mlir::Value address = storage.lookup(ref->decl);
    auto type = mlir::cast<mlir::MemRefType>(address.getType());
    if (value.getType() != type.getElementType()) {
      diag.error(binary.loc, "MLIR assignment types do not match the AST");
      return {};
    }
    builder.create<mlir::memref::StoreOp>(builder.getUnknownLoc(), value,
                                          address, mlir::ValueRange{});
    return value;
  }
  if (binary.op >= BinaryOp::EQ && binary.op <= BinaryOp::GEQ) {
    if (!binary.lhs || !binary.rhs || !binary.lhs->type || !binary.rhs->type ||
        !binary.lhs->type->isIntegral() || !binary.rhs->type->isIntegral() ||
        !binary.type || binary.type->kind != Type::INT) {
      diag.error(binary.loc,
                 "MLIR comparisons currently require integer operands");
      return {};
    }

    mlir::Value lhs = emitExpr(*binary.lhs);
    if (!lhs)
      return {};

    mlir::Value rhs = emitExpr(*binary.rhs);
    if (!rhs)
      return {};

    if (!mlir::isa<mlir::IntegerType>(lhs.getType()) ||
        lhs.getType() != rhs.getType()) {
      diag.error(binary.loc,
                 "MLIR comparison operand types do not match the AST");
      return {};
    }

    const bool isSigned = binary.lhs->type->isSigned();
    using Predicate = mlir::arith::CmpIPredicate;
    Predicate predicate = Predicate::eq;

    switch (binary.op) {
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

    mlir::Type resultType = lowerType(*binary.type, binary.loc);
    if (!resultType)
      return {};

    auto loc = builder.getUnknownLoc();
    mlir::Value comparison =
        builder.create<mlir::arith::CmpIOp>(loc, predicate, lhs, rhs)
            .getResult();

    return builder.create<mlir::arith::ExtUIOp>(loc, resultType, comparison)
        .getResult();
  }
  if (binary.op != BinaryOp::ADD && binary.op != BinaryOp::SUB &&
      binary.op != BinaryOp::MUL) {
    diag.error(binary.loc,
               "MLIR generation does not support this binary operator yet");
    return {};
  }
  if (!binary.type || !binary.type->isIntegral() || !binary.lhs ||
      !binary.rhs) {
    diag.error(binary.loc,
               "MLIR binary arithmetic currently requires integer operands");
    return {};
  }
  mlir::Value lhs = emitExpr(*binary.lhs);
  if (!lhs)
    return {};
  mlir::Value rhs = emitExpr(*binary.rhs);
  if (!rhs)
    return {};
  mlir::Type resultType = lowerType(*binary.type, binary.loc);
  if (!resultType || !mlir::isa<mlir::IntegerType>(lhs.getType()) ||
      lhs.getType() != rhs.getType() || lhs.getType() != resultType) {
    diag.error(binary.loc, "MLIR binary operand types do not match the AST");
    return {};
  }
  auto loc = builder.getUnknownLoc();
  switch (binary.op) {
  case BinaryOp::ADD:
    return builder.create<mlir::arith::AddIOp>(loc, lhs, rhs).getResult();
  case BinaryOp::SUB:
    return builder.create<mlir::arith::SubIOp>(loc, lhs, rhs).getResult();
  case BinaryOp::MUL:
    return builder.create<mlir::arith::MulIOp>(loc, lhs, rhs).getResult();
  default:
    break;
  }
  diag.error(binary.loc,
             "MLIR generation does not support this expression yet");
  return {};
}
