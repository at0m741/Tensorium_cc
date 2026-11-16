#pragma once

#include "mopt/Analysis/LoopAnalysis.h"

#include "mlir/Dialect/SCF/IR/SCF.h"

#include <vector>

namespace mopt {

std::vector<DependenceInfo>
analyzeDependences(mlir::scf::ForOp loop,
                   const std::vector<MemoryAccessInfo> &accesses);

} // namespace mopt
