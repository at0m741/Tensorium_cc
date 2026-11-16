#pragma once

namespace llvm {
class raw_ostream;
}

namespace mopt {
struct LoopInfo;
struct VectorizationPlan;

void printLoopReport(llvm::raw_ostream &os, unsigned loopId,
                     const LoopInfo &info, const VectorizationPlan &plan);
} // namespace mopt
