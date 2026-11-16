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

mlir::LogicalResult tensorium::ArrayDecayOp::verify() {
  auto array = mlir::dyn_cast<ArrayType>(getBase().getType().getPointeeType());
  if (!array ||
      array.getElementType() != getResult().getType().getPointeeType())
    return emitOpError(
        "requires a pointer to an array and a matching element pointer");
  return mlir::success();
}

mlir::LogicalResult tensorium::PtrCmpOp::verify() {
  auto predicate = getPredicate();
  if (predicate != "eq" && predicate != "ne" && predicate != "ult" &&
      predicate != "ule" && predicate != "ugt" && predicate != "uge")
    return emitOpError("expected eq, ne, ult, ule, ugt or uge predicate");
  return mlir::success();
}
