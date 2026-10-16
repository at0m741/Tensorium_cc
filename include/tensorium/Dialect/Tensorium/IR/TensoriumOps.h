#pragma once

#include "tensorium/Dialect/Tensorium/IR/TensoriumTypes.h"

#include "mlir/IR/OpDefinition.h"
#include "mlir/Interfaces/SideEffectInterfaces.h"

#define GET_OP_CLASSES
#include "tensorium/Dialect/Tensorium/IR/TensoriumOps.h.inc"
