#include "tensorium/Dialect/Tensorium/IR/TensoriumDialect.h"
#include "tensorium/Dialect/Tensorium/IR/TensoriumOps.h"

#include "tensorium/Dialect/Tensorium/IR/TensoriumDialect.cpp.inc"

void tensorium::TensoriumDialect::initialize() {
  registerTypes();

  addOperations<
#define GET_OP_LIST
#include "tensorium/Dialect/Tensorium/IR/TensoriumOps.cpp.inc"
      >();
}
