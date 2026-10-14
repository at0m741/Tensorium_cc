#pragma once

#include "cc1/AST/Type.hpp"
#include "mlir/IR/Types.h"
#include "mlir/IR/Value.h"

struct Address {
  mlir::Value base;
  mlir::Type elementType;
  const Type *objectType = nullptr;

  explicit operator bool() const { return static_cast<bool>(base); }
};
