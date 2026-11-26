#include "tensorium/Dialect/Tensorium/IR/TensoriumAttrs.h"
#include "tensorium/Dialect/Tensorium/IR/TensoriumDialect.h"
#include "tensorium/Dialect/Tensorium/IR/TensoriumTypes.h"

#include "mlir/IR/Builders.h"
#include "mlir/IR/DialectImplementation.h"
#include "llvm/ADT/TypeSwitch.h"

#define GET_ATTRDEF_CLASSES
#include "tensorium/Dialect/Tensorium/IR/TensoriumAttrs.cpp.inc"

void tensorium::TensoriumDialect::registerAttributes() {
  addAttributes<
#define GET_ATTRDEF_LIST
#include "tensorium/Dialect/Tensorium/IR/TensoriumAttrs.cpp.inc"
      >();
}

mlir::FailureOr<mlir::Type>
tensorium::GlobalAddressAttr::getPointeeType(mlir::Type root) const {
  auto indices = getIndices();
  if (indices.empty())
    return root;
  if (indices.front() < 0 || indices.front() > 1 ||
      (indices.front() == 1 && indices.size() > 1))
    return mlir::failure();
  for (size_t i = 1; i < indices.size(); ++i) {
    auto array = mlir::dyn_cast<ArrayType>(root);
    if (!array || indices[i] < 0 ||
        static_cast<uint64_t>(indices[i]) > array.getSize() ||
        (static_cast<uint64_t>(indices[i]) == array.getSize() &&
         i + 1 < indices.size()))
      return mlir::failure();
    root = array.getElementType();
  }
  return root;
}
