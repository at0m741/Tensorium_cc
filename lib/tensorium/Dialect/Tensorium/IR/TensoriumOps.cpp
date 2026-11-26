#include "tensorium/Dialect/Tensorium/IR/TensoriumOps.h"

#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/OpImplementation.h"

#include "tensorium/Dialect/Tensorium/IR/TensoriumEnums.cpp.inc"

#define GET_OP_CLASSES
#include "tensorium/Dialect/Tensorium/IR/TensoriumOps.cpp.inc"

mlir::LogicalResult tensorium::AllocaOp::verify() {
  if (mlir::isa<VoidType>(getResult().getType().getPointeeType()))
    return emitOpError("cannot allocate a void object");
  if (!getOperation()
           ->getParentWithTrait<mlir::OpTrait::AutomaticAllocationScope>()) {
    return emitOpError("requires an enclosing automatic allocation scope");
  }

  return mlir::success();
}

mlir::LogicalResult tensorium::LoadOp::verify() {
  if (mlir::isa<VoidType>(getAddress().getType().getPointeeType()))
    return emitOpError("cannot load a void object");
  return mlir::success();
}

mlir::LogicalResult tensorium::StoreOp::verify() {
  if (mlir::isa<VoidType>(getAddress().getType().getPointeeType()))
    return emitOpError("cannot store a void object");
  return mlir::success();
}

mlir::LogicalResult tensorium::PtrCastOp::verify() {
  auto from = getSource().getType().getPointeeType();
  auto to = getResult().getType().getPointeeType();
  if (from != to && !mlir::isa<VoidType>(from) && !mlir::isa<VoidType>(to))
    return emitOpError(
        "requires identical pointees or a void pointer endpoint");
  return mlir::success();
}

mlir::LogicalResult tensorium::PtrOffsetOp::verify() {
  if (mlir::isa<VoidType>(getBase().getType().getPointeeType()))
    return emitOpError("cannot offset a void pointer");
  return mlir::success();
}

mlir::LogicalResult tensorium::PtrDiffOp::verify() {
  if (mlir::isa<VoidType>(getLhs().getType().getPointeeType()))
    return emitOpError("cannot subtract void pointers");
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

mlir::LogicalResult tensorium::GlobalOp::verify() {
  if (!mlir::isa<mlir::ModuleOp>((*this)->getParentOp()))
    return emitOpError("must be directly nested in a module");
  auto type = getObjectType();
  if (!mlir::isa<mlir::IntegerType, mlir::FloatType, PtrType, ArrayType>(type))
    return emitOpError("requires a supported object type");
  auto initial = getInitialValue();
  if (!initial) {
    if (getLinkage() == GlobalLinkage::internal)
      return emitOpError("internal linkage requires a definition");
    return mlir::success();
  }
  if (mlir::isa<mlir::UnitAttr>(*initial))
    return mlir::success();
  if (mlir::isa<GlobalAddressAttr>(*initial)) {
    if (!mlir::isa<PtrType>(type))
      return emitOpError(
          "global address initializer requires a pointer object");
    return mlir::success();
  }
  auto integer = mlir::dyn_cast<mlir::IntegerAttr>(*initial);
  if (!integer || integer.getType() != type)
    return emitOpError(
        "requires a matching integer initializer or unit zero initializer");
  return mlir::success();
}

mlir::LogicalResult
tensorium::GlobalOp::verifySymbolUses(mlir::SymbolTableCollection &symbols) {
  auto initial = getInitialValue();
  auto address = initial ? mlir::dyn_cast<GlobalAddressAttr>(*initial)
                         : GlobalAddressAttr{};
  if (!address)
    return mlir::success();
  auto target =
      symbols.lookupNearestSymbolFrom<GlobalOp>(*this, address.getGlobal());
  if (!target)
    return emitOpError(
        "address initializer references an undefined Tensorium global");
  auto pointer = mlir::dyn_cast<PtrType>(getObjectType());
  if (!pointer || (pointer.getPointeeType() != target.getObjectType() &&
                   !mlir::isa<VoidType>(pointer.getPointeeType())))
    return emitOpError("address initializer pointee type does not match the "
                       "referenced global");
  return mlir::success();
}

mlir::LogicalResult
tensorium::GetGlobalOp::verifySymbolUses(mlir::SymbolTableCollection &symbols) {
  auto global = symbols.lookupNearestSymbolFrom<GlobalOp>(*this, getNameAttr());
  if (!global)
    return emitOpError("references an undefined Tensorium global");
  if (global.getObjectType() != getResult().getType().getPointeeType())
    return emitOpError(
        "pointer pointee type does not match the global object type");
  return mlir::success();
}
