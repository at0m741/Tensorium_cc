#pragma once

#include <memory>

namespace mlir {
class Pass;
}

namespace tensorium {

// Compatibility lowering for pointers to integer/float objects. Internal
// functions and CFG edges use rank-zero memref descriptors after conversion.
// Pointer-valued storage and external pointer ABIs are deliberately
// unsupported.
std::unique_ptr<mlir::Pass> createTensoriumToMemRefPass();
void registerTensoriumToMemRefPass();

} // namespace tensorium
