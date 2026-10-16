#include "tensorium/Dialect/Tensorium/IR/TensoriumOps.h"

#include "mlir/IR/Builders.h"
#include "mlir/IR/OpImplementation.h"

#define GET_OP_CLASSES
#include "tensorium/Dialect/Tensorium/IR/TensoriumOps.cpp.inc"

mlir::LogicalResult tensorium::AllocaOp::verify() {
  if (!getOperation()
           ->getParentWithTrait<mlir::OpTrait::AutomaticAllocationScope>()) {
    return emitOpError("requires an enclosing automatic allocation scope");
  }

  return mlir::success();
}
