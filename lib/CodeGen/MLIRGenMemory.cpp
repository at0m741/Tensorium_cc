#include "cc1/CodeGen/MLIRGen.hpp"
#include "mlir/Dialect/MemRef/IR/MemRef.h"

mlir::Value MLIRGen::createScalarStorage(mlir::Type type) {
  return mlir::memref::AllocaOp::create(builder, builder.getUnknownLoc(),
                                        mlir::MemRefType::get({}, type))
      .getResult();
}

mlir::Value MLIRGen::emitLValue(const Expr &expr) {
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
  auto address = createScalarStorage(value->second.getType());
  emitStore(value->second, address);
  storage[reference->decl] = address;
  return address;
}

mlir::Value MLIRGen::emitLoad(mlir::Value address) {
  return mlir::memref::LoadOp::create(builder, builder.getUnknownLoc(), address,
                                      mlir::ValueRange{})
      .getResult();
}

void MLIRGen::emitStore(mlir::Value value, mlir::Value address) {
  mlir::memref::StoreOp::create(builder, builder.getUnknownLoc(), value,
                                address, mlir::ValueRange{});
}

void MLIRGen::materializeParameters() {
  for (const auto &binding : values) {
    if (storage.find(binding.first) != storage.end())
      continue;
    auto address = createScalarStorage(binding.second.getType());
    emitStore(binding.second, address);
    storage[binding.first] = address;
  }
}
