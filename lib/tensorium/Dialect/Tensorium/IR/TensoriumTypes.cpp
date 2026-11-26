#include "tensorium/Dialect/Tensorium/IR/TensoriumTypes.h"
#include "tensorium/Dialect/Tensorium/IR/TensoriumDialect.h"

#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/DialectImplementation.h"

#include "llvm/ADT/STLFunctionalExtras.h"
#include "llvm/ADT/TypeSwitch.h"
#include "llvm/Support/Casting.h"

#define GET_TYPEDEF_CLASSES
#include "tensorium/Dialect/Tensorium/IR/TensoriumTypes.cpp.inc"

namespace tensorium {

void TensoriumDialect::registerTypes() {
  addTypes<
#define GET_TYPEDEF_LIST
#include "tensorium/Dialect/Tensorium/IR/TensoriumTypes.cpp.inc"
      >();
}

mlir::LogicalResult
PtrType::verify(llvm::function_ref<mlir::InFlightDiagnostic()> emitError,
                mlir::Type pointeeType) {
  if (!pointeeType) {
    emitError() << "expected a pointee type";
    return mlir::failure();
  }

  if (!llvm::isa<mlir::IntegerType, mlir::FloatType, PtrType, ArrayType,
                 VoidType>(pointeeType)) {
    emitError() << "expected an integer, floating-point, array, void, or "
                   "Tensorium pointer "
                   "pointee type, but got "
                << pointeeType;
    return mlir::failure();
  }

  return mlir::success();
}

mlir::LogicalResult
ArrayType::verify(llvm::function_ref<mlir::InFlightDiagnostic()> emitError,
                  uint64_t size, mlir::Type elementType) {
  if (size == 0) {
    emitError() << "array size must be positive";
    return mlir::failure();
  }
  if (!llvm::isa_and_nonnull<mlir::IntegerType, mlir::FloatType, PtrType,
                             ArrayType>(elementType)) {
    emitError() << "expected a supported array element type";
    return mlir::failure();
  }
  return mlir::success();
}

} // namespace tensorium
