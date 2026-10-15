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

  if (!llvm::isa<mlir::IntegerType, mlir::FloatType, PtrType>(pointeeType)) {
    emitError() << "expected an integer, floating-point, or Tensorium pointer "
                   "pointee type, but got "
                << pointeeType;
    return mlir::failure();
  }

  return mlir::success();
}

} // namespace tensorium
