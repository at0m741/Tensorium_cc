#include "cc1/CodeGen/MLIRGen.hpp"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/IR/BuiltinTypes.h"
#include "tensorium/Dialect/Tensorium/IR/TensoriumOps.h"
#include "tensorium/Support/Builders.h"

mlir::Value MLIRGen::emitBinary(const BinaryExpr &binary) {
  using tensorium::createOp;
  auto loc = builder.getUnknownLoc();
  if ((binary.op == BinaryOp::ADD_ASSIGN ||
       binary.op == BinaryOp::SUB_ASSIGN) &&
      binary.lhs->type->isPointer()) {
    Address address = emitLValue(*binary.lhs);
    if (!address)
      return {};
    auto value = emitPointerOffset(emitLoad(address), *binary.rhs,
                                   binary.op == BinaryOp::SUB_ASSIGN);
    if (value)
      emitStore(value, address);
    return value;
  }
  if (binary.op == BinaryOp::ADD_ASSIGN || binary.op == BinaryOp::SUB_ASSIGN ||
      binary.op == BinaryOp::MUL_ASSIGN) {
    if (!binary.computationType || !binary.computationType->isIntegral() ||
        !binary.lhs->type->isIntegral() || !binary.rhs->type->isIntegral()) {
      diag.error(binary.loc,
                 "MLIR compound arithmetic requires integer operands");
      return {};
    }
    // Preserve the address once, then perform arithmetic at Sema's promoted
    // computation type and convert back to the stored/returned object type.
    Address address = emitLValue(*binary.lhs);
    if (!address)
      return {};
    auto lhs = emitIntegerConversion(emitLoad(address), *binary.lhs->type,
                                     *binary.computationType, binary.loc);
    if (!lhs)
      return {};
    auto rhs = emitIntegerConversion(emitExpr(*binary.rhs), *binary.rhs->type,
                                     *binary.computationType, binary.loc);
    if (!rhs)
      return {};
    mlir::Value result;
    if (binary.op == BinaryOp::ADD_ASSIGN)
      result =
          createOp<mlir::arith::AddIOp>(builder, loc, lhs, rhs).getResult();
    else if (binary.op == BinaryOp::SUB_ASSIGN)
      result =
          createOp<mlir::arith::SubIOp>(builder, loc, lhs, rhs).getResult();
    else
      result =
          createOp<mlir::arith::MulIOp>(builder, loc, lhs, rhs).getResult();
    result = emitIntegerConversion(result, *binary.computationType,
                                   *binary.lhs->type, binary.loc);
    if (result)
      emitStore(result, address);
    return result;
  }
  if ((binary.op == BinaryOp::ADD || binary.op == BinaryOp::SUB) &&
      binary.type->isPointer()) {
    const Expr *pointer = binary.lhs;
    const Expr *index = binary.rhs;
    if (!pointer->type->isPointer())
      std::swap(pointer, index);
    auto base = emitExpr(*pointer);
    return base ? emitPointerOffset(base, *index, binary.op == BinaryOp::SUB)
                : mlir::Value{};
  }
  if (binary.lhs->type->isPointer() && binary.rhs->type->isPointer()) {
    auto lhs = emitExpr(*binary.lhs);
    auto rhs = emitExpr(*binary.rhs);
    if (!lhs || !rhs)
      return {};
    if (binary.op == BinaryOp::SUB)
      return createOp<tensorium::PtrDiffOp>(
                 builder, loc, lowerType(*binary.type, binary.loc), lhs, rhs)
          .getResult();
    const char *predicate = nullptr;
    switch (binary.op) {
    case BinaryOp::EQ:
      predicate = "eq";
      break;
    case BinaryOp::NEQ:
      predicate = "ne";
      break;
    case BinaryOp::LT:
      predicate = "ult";
      break;
    case BinaryOp::LEQ:
      predicate = "ule";
      break;
    case BinaryOp::GT:
      predicate = "ugt";
      break;
    case BinaryOp::GEQ:
      predicate = "uge";
      break;
    default:
      break;
    }
    if (predicate) {
      auto comparison = createOp<tensorium::PtrCmpOp>(
          builder, loc, builder.getI1Type(), builder.getStringAttr(predicate),
          lhs, rhs);
      return createOp<mlir::arith::ExtUIOp>(builder, loc,
                                            lowerType(*binary.type, binary.loc),
                                            comparison.getResult())
          .getResult();
    }
  }
  if (binary.op == BinaryOp::ASSIGN) {
    Address address = emitLValue(*binary.lhs);
    if (!address)
      return {};

    mlir::Value value = emitExpr(*binary.rhs);
    if (!value)
      return {};

    if (value.getType() != address.elementType) {
      diag.error(binary.loc, "MLIR assignment types do not match the AST");
      return {};
    }

    emitStore(value, address);
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
#if LLVM_VERSION_MAJOR >= 22
    mlir::Value comparison =
        mlir::arith::CmpIOp::create(builder, loc, predicate, lhs, rhs)
            .getResult();
#else
    mlir::Value comparison =
        builder.create<mlir::arith::CmpIOp>(loc, predicate, lhs, rhs)
            .getResult();
#endif

#if LLVM_VERSION_MAJOR >= 22
    return mlir::arith::ExtUIOp::create(builder, loc, resultType, comparison)
        .getResult();
#else
    return builder.create<mlir::arith::ExtUIOp>(loc, resultType, comparison)
        .getResult();
#endif
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
  switch (binary.op) {
  case BinaryOp::ADD:
#if LLVM_VERSION_MAJOR >= 22
    return mlir::arith::AddIOp::create(builder, loc, lhs, rhs).getResult();
#else
    return builder.create<mlir::arith::AddIOp>(loc, lhs, rhs).getResult();
#endif
  case BinaryOp::SUB:
#if LLVM_VERSION_MAJOR >= 22
    return mlir::arith::SubIOp::create(builder, loc, lhs, rhs).getResult();
#else
    return builder.create<mlir::arith::SubIOp>(loc, lhs, rhs).getResult();
#endif
  case BinaryOp::MUL:
#if LLVM_VERSION_MAJOR >= 22
    return mlir::arith::MulIOp::create(builder, loc, lhs, rhs).getResult();
#else
    return builder.create<mlir::arith::MulIOp>(loc, lhs, rhs).getResult();
#endif
  default:
    break;
  }
  diag.error(binary.loc,
             "MLIR generation does not support this expression yet");
  return {};
}
