#pragma once
#include "mlir/Bytecode/BytecodeOpInterface.h"

#include "tensorium/Dialect/Tensorium/IR/TensoriumTypes.h"
#include "tensorium/Dialect/Tensorium/IR/TensoriumAttrs.h"

#include "mlir/IR/OpDefinition.h"
#include "mlir/IR/SymbolTable.h"
#include "tensorium/Dialect/Tensorium/IR/TensoriumEnums.h"
#include "mlir/Interfaces/SideEffectInterfaces.h"

#define GET_OP_CLASSES
#include "tensorium/Dialect/Tensorium/IR/TensoriumOps.h.inc"
