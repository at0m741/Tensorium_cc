#pragma once

#include "mlir/Pass/Pass.h"
#include "mopt/Target/TargetInfo.h"
#include <memory>

namespace mopt {

// The caller owns the report stream; it must outlive the pass manager.
// Analysis preserves the input IR. The CLI-registered pass reports to stderr.
std::unique_ptr<mlir::Pass> createLoopAnalysisPass(TargetInfo targetInfo,
                                                   llvm::raw_ostream &report);

void registerMOptPasses();

} // namespace mopt
