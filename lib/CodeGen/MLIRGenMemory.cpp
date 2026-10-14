#include "cc1/CodeGen/MLIRGen.hpp"
#include "mlir/Dialect/MemRef/IR/MemRef.h"

Address MLIRGen::createScalarStorage(mlir::Type type, const Type &objectType) {
  auto allocation = mlir::memref::AllocaOp::create(
      builder, builder.getUnknownLoc(), mlir::MemRefType::get({}, type));

  return {allocation.getResult(), type, &objectType};
}

Address MLIRGen::emitLValue(const Expr &expr) {
  auto *reference = dynamic_cast<const IdentExpr *>(&expr);
  if (!reference || !reference->decl) {
    diag.error(expr.loc, "MLIR assignment requires a scalar variable");
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
  return mlir::memref::LoadOp::create(builder, builder.getUnknownLoc(),
                                      address.base, mlir::ValueRange{})
      .getResult();
}

void MLIRGen::emitStore(mlir::Value value, Address address) {
  mlir::memref::StoreOp::create(builder, builder.getUnknownLoc(), value,
                                address.base, mlir::ValueRange{});
}

void MLIRGen::materializeParameters() {
  for (const auto &binding : values) {
    if (storage.find(binding.first) != storage.end())
      continue;

    auto address =
        createScalarStorage(binding.second.getType(), *binding.first->type);
    emitStore(binding.second, address);
    storage[binding.first] = address;
  }
}
