#include "tensorium/Dialect/Tensorium/IR/TensoriumDialect.h"

#include "tensorium/Dialect/Tensorium/IR/TensoriumDialect.cpp.inc"

void tensorium::TensoriumDialect::initialize() {
  registerTypes();
}
