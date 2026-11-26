#include "tensorium/Dialect/Tensorium/IR/TensoriumAttrs.h"
#include "tensorium/Dialect/Tensorium/IR/TensoriumDialect.h"

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
