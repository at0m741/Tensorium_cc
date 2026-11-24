#include "cc1/CodeGen/MLIRGen.hpp"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "tensorium/Dialect/Tensorium/IR/TensoriumOps.h"
#include "tensorium/Support/Builders.h"

using tensorium::createOp;

Address MLIRGen::createObjectStorage(mlir::func::FuncOp function,
                                     mlir::Type type, const Type &objectType) {
  mlir::OpBuilder::InsertionGuard guard(builder);
  builder.setInsertionPointToStart(&function.getBody().front());

#if LLVM_VERSION_MAJOR >= 22
  auto allocation = tensorium::AllocaOp::create(
      builder, builder.getUnknownLoc(),
      tensorium::PtrType::get(builder.getContext(), type));
#else
  auto allocation = builder.create<tensorium::AllocaOp>(
      builder.getUnknownLoc(),
      tensorium::PtrType::get(builder.getContext(), type));
#endif

  return {allocation.getResult(), type, &objectType};
}

Address MLIRGen::emitLValue(const Expr &expr) {
  if (auto *index = dynamic_cast<const IndexExpr *>(&expr)) {
    const Expr *pointer = index->base;
    const Expr *offset = index->index;
    if (pointer->type->isIntegral())
      std::swap(pointer, offset);
    mlir::Value base = emitExpr(*pointer);
    if (!base)
      return {};
    mlir::Value address = emitPointerOffset(base, *offset);
    if (!address)
      return {};
    return {address,
            mlir::cast<tensorium::PtrType>(address.getType()).getPointeeType(),
            expr.type};
  }
  if (auto *unary = dynamic_cast<const UnaryExpr *>(&expr)) {
    if (unary->op == UnaryOp::DEREF) {
      mlir::Value pointer = emitExpr(*unary->operand);
      if (!pointer)
        return {};

      auto pointerType = mlir::dyn_cast<tensorium::PtrType>(pointer.getType());

      if (!pointerType || !expr.type) {
        diag.error(expr.loc,
                   "MLIR dereference requires a typed Tensorium pointer");
        return {};
      }

      mlir::Type objectType = lowerType(*expr.type, expr.loc);
      if (!objectType)
        return {};

      if (pointerType.getPointeeType() != objectType) {
        diag.error(expr.loc, "MLIR dereference type does not match the AST");
        return {};
      }

      return {pointer, objectType, expr.type};
    }
  }

  auto *reference = dynamic_cast<const IdentExpr *>(&expr);
  if (!reference || !reference->decl) {
    diag.error(expr.loc,
               "MLIR lvalue requires a variable, index or dereference");
    return {};
  }

  auto slot = storage.find(reference->decl);
  if (slot != storage.end())
    return slot->second;

  if (values.count(reference->decl)) {
    diag.error(reference->loc,
               "MLIR storage analysis did not provide storage for this lvalue");
  } else {
    diag.error(reference->loc,
               "unresolved assignment target in MLIR generation");
  }
  return {};
}

mlir::Value MLIRGen::emitLoad(Address address) {
#if LLVM_VERSION_MAJOR >= 22
  return tensorium::LoadOp::create(builder, builder.getUnknownLoc(),
                                   address.elementType, address.base)
      .getResult();
#else
  return builder
      .create<tensorium::LoadOp>(builder.getUnknownLoc(), address.elementType,
                                 address.base)
      .getResult();
#endif
}

void MLIRGen::emitStore(mlir::Value value, Address address) {
#if LLVM_VERSION_MAJOR >= 22
  tensorium::StoreOp::create(builder, builder.getUnknownLoc(), value,
                             address.base);
#else
  builder.create<tensorium::StoreOp>(builder.getUnknownLoc(), value,
                                     address.base);
#endif
}

mlir::Value MLIRGen::emitPointerOffset(mlir::Value base, const Expr &offset,
                                       bool subtract) {
  mlir::Value index = emitExpr(offset);
  if (!index)
    return {};
  auto from = mlir::dyn_cast<mlir::IntegerType>(index.getType());
  if (!mlir::isa<tensorium::PtrType>(base.getType()) || !from) {
    diag.error(offset.loc,
               "MLIR pointer offset requires a pointer and an integer");
    return {};
  }
  auto to = builder.getIntegerType(target.sizeofPointer * 8);
  auto loc = builder.getUnknownLoc();
  if (from.getWidth() < to.getWidth()) {
    index = offset.type->isSigned()
                ? createOp<mlir::arith::ExtSIOp>(builder, loc, to, index)
                      .getResult()
                : createOp<mlir::arith::ExtUIOp>(builder, loc, to, index)
                      .getResult();
  } else if (from.getWidth() > to.getWidth()) {
    index =
        createOp<mlir::arith::TruncIOp>(builder, loc, to, index).getResult();
  }
  if (subtract) {
    auto zero = createOp<mlir::arith::ConstantOp>(
        builder, loc, builder.getIntegerAttr(to, 0));
    index = createOp<mlir::arith::SubIOp>(builder, loc, zero.getResult(), index)
                .getResult();
  }
  return createOp<tensorium::PtrOffsetOp>(builder, loc, base.getType(), base,
                                          index)
      .getResult();
}

mlir::Value MLIRGen::emitUpdate(const UnaryExpr &expr) {
  Address address = emitLValue(*expr.operand);
  if (!address)
    return {};
  mlir::Value old = emitLoad(address);
  mlir::Value next;
  auto loc = builder.getUnknownLoc();
  const bool decrement =
      expr.op == UnaryOp::PRE_DEC || expr.op == UnaryOp::POST_DEC;
  if (expr.type->isPointer()) {
    auto integer = builder.getIntegerType(target.sizeofPointer * 8);
    auto one = createOp<mlir::arith::ConstantOp>(
        builder, loc, builder.getIntegerAttr(integer, decrement ? -1 : 1));
    next = createOp<tensorium::PtrOffsetOp>(builder, loc, old.getType(), old,
                                            one.getResult())
               .getResult();
  } else if (expr.type->isIntegral() && expr.type->kind != Type::BOOL) {
    auto one = createOp<mlir::arith::ConstantOp>(
        builder, loc, builder.getIntegerAttr(old.getType(), 1));
    next =
        decrement
            ? createOp<mlir::arith::SubIOp>(builder, loc, old, one.getResult())
                  .getResult()
            : createOp<mlir::arith::AddIOp>(builder, loc, old, one.getResult())
                  .getResult();
  } else {
    diag.error(expr.loc,
               "MLIR increment/decrement requires an integer or pointer");
    return {};
  }
  emitStore(next, address);
  return expr.op == UnaryOp::POST_INC || expr.op == UnaryOp::POST_DEC ? old
                                                                      : next;
}
