#include "mlir/IR/BuiltinOps.h"
#include "mlir/Pass/PassRegistry.h"
#include "mopt/Analysis/LoopAnalysis.h"
#include "mopt/Passes.h"
#include "mopt/Planning/LoopReport.h"
#include "mopt/Planning/VectorizationPlanner.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"
#include <optional>
#include <utility>

namespace {

class LoopAnalysisPass
    : public mlir::PassWrapper<LoopAnalysisPass,
                               mlir::OperationPass<mlir::ModuleOp>> {
public:
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(LoopAnalysisPass)

  LoopAnalysisPass() = default;
  LoopAnalysisPass(mopt::TargetInfo targetInfo, llvm::raw_ostream &report)
      : targetInfo(std::move(targetInfo)), report(&report) {}
  LoopAnalysisPass(const LoopAnalysisPass &other)
      : PassWrapper(other), targetInfo(other.targetInfo), report(other.report) {
  }

  llvm::StringRef getArgument() const final { return "mopt-analyze-loops"; }
  llvm::StringRef getDescription() const final {
    return "Report SCF loop analyses and target-aware vectorization plans";
  }

  void runOnOperation() override {
    auto resolved = targetInfo;
    if (!resolved) {
      auto queried = mopt::queryTargetInfo({triple, cpu, features});
      if (!queried) {
        getOperation().emitError(
            "MOpt failed to query LLVM target information: ")
            << llvm::toString(queried.takeError());
        signalPassFailure();
        return;
      }
      resolved = std::move(*queried);
    }

    unsigned loopId = 0;
    getOperation().walk([&](mlir::scf::ForOp loop) {
      auto info = mopt::analyzeLoop(loop);
      auto plan = mopt::planVectorization(info, *resolved);
      mopt::printLoopReport(*report, loopId++, info, plan);
    });
    markAllAnalysesPreserved();
  }

private:
  Option<std::string> triple{*this, "target-triple",
                             llvm::cl::desc("LLVM triple (defaults to host)"),
                             llvm::cl::init("")};
  Option<std::string> cpu{*this, "mcpu",
                          llvm::cl::desc("LLVM CPU (defaults to host CPU)"),
                          llvm::cl::init("")};
  Option<std::string> features{*this, "mattr",
                               llvm::cl::desc("LLVM target feature string"),
                               llvm::cl::init("")};
  std::optional<mopt::TargetInfo> targetInfo;
  llvm::raw_ostream *report = &llvm::errs();
};

} // namespace

std::unique_ptr<mlir::Pass>
mopt::createLoopAnalysisPass(TargetInfo targetInfo, llvm::raw_ostream &report) {
  return std::make_unique<LoopAnalysisPass>(std::move(targetInfo), report);
}

void mopt::registerMOptPasses() {
  static mlir::PassRegistration<LoopAnalysisPass> registration;
}
