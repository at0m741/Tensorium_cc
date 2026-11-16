#pragma once

#include "mlir/IR/Builders.h"
#include "llvm/Config/llvm-config.h"
#include <utility>

namespace tensorium {
// Keep the MLIR builder API transition out of individual lowering patterns.
template <typename Op, typename... Args>
Op createOp(mlir::OpBuilder &builder, mlir::Location loc, Args &&...args) {
#if LLVM_VERSION_MAJOR >= 22
  return Op::create(builder, loc, std::forward<Args>(args)...);
#else
  return builder.create<Op>(loc, std::forward<Args>(args)...);
#endif
}
} // namespace tensorium
