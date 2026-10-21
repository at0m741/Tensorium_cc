#include "cc1/CodeGen/MLIRGen.hpp"
#include "tensorium/Dialect/Tensorium/IR/TensoriumOps.h"

Address MLIRGen::createScalarStorage(mlir::Type type, const Type &objectType) {
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
               "MLIR lvalue requires a scalar variable or dereference");
    return {};
  }

  if (reference->decl->type->isPointer()) {
    diag.error(expr.loc, "MLIR pointer-valued storage is not supported yet");
    return {};
  }

  auto slot = storage.find(reference->decl);
  if (slot != storage.end())
    return slot->second;

  auto value = values.find(reference->decl);
  if (value == values.end()) {
    diag.error(reference->loc,
               "unresolved assignment target in MLIR generation");
    return {};
  }

  auto address =
      createScalarStorage(value->second.getType(), *reference->decl->type);
  emitStore(value->second, address);
  storage[reference->decl] = address;
  return address;
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

void MLIRGen::materializeParameters() {
  for (const auto &binding : values) {
    // Pointer parameters remain SSA values for this first milestone.
    if (binding.first->type->isPointer())
      continue;

    if (storage.find(binding.first) != storage.end())
      continue;

    auto address =
        createScalarStorage(binding.second.getType(), *binding.first->type);
    emitStore(binding.second, address);
    storage[binding.first] = address;
  }
}
