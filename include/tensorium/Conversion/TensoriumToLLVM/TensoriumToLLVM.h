#pragma once

#include <memory>

namespace mlir {
class Pass;
}

namespace tensorium {
// Lower Tensorium object memory and arith/cf/func to LLVM. The input module
// must carry an explicit llvm.data_layout. Ordinary C pointers use the native
// LLVM pointer ABI; this is an unprotected lowering.
std::unique_ptr<mlir::Pass> createTensoriumToLLVMPass();
void registerTensoriumToLLVMPass();
} // namespace tensorium
