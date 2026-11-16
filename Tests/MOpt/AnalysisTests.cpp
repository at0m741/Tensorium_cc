#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/Parser/Parser.h"
#include "mlir/Pass/PassManager.h"
#include "mopt/Analysis/LoopAnalysis.h"
#include "mopt/Passes.h"
#include "mopt/Planning/VectorizationPlanner.h"
#include "llvm/Support/raw_ostream.h"

#define CHECK(condition)                                                       \
  do {                                                                         \
    if (!(condition)) {                                                        \
      llvm::errs() << "Failed: " << #condition << "\n";                        \
      return 1;                                                                \
    }                                                                          \
  } while (false)

static std::string printModule(mlir::ModuleOp module) {
  std::string text;
  llvm::raw_string_ostream stream(text);
  module.print(stream);
  return stream.str();
}

int main(int argc, char **argv) {
  CHECK(argc == 2);
  mlir::DialectRegistry registry;
  registry.insert<mlir::arith::ArithDialect, mlir::func::FuncDialect,
                  mlir::memref::MemRefDialect, mlir::scf::SCFDialect>();
  mlir::MLIRContext context(registry);
  auto module = mlir::parseSourceFile<mlir::ModuleOp>(argv[1], &context);
  CHECK(module);
  const auto before = printModule(*module);
  mlir::scf::ForOp loop;
  module->walk([&](mlir::scf::ForOp found) { loop = found; });
  CHECK(loop);

  const auto info = mopt::analyzeLoop(loop);
  CHECK(info.tripCount == 4096);
  CHECK(info.loads == 2 && info.stores == 1);
  CHECK(info.legality.vectorization == mopt::LegalityState::Unknown);
  mopt::TargetInfo target;
  target.fixedVectorRegisterBits = 128;
  const auto wide = mopt::planVectorization(info, target);
  CHECK(wide.naturalFactor == 4);
  CHECK(wide.decision == mopt::VectorizationDecision::Unknown);
  target.fixedVectorRegisterBits = 64;
  CHECK(mopt::planVectorization(info, target).naturalFactor == 2);

  // Exercise the reusable pass with an injected target and a caller-owned
  // report stream, without querying the machine running this test.
  std::string report;
  llvm::raw_string_ostream reportStream(report);
  mlir::PassManager manager(&context);
  manager.addPass(mopt::createLoopAnalysisPass(target, reportStream));
  CHECK(mlir::succeeded(manager.run(*module)));
  CHECK(report.find("mopt.loop #0") != std::string::npos);
  CHECK(report.find("fixed-vector-bits = 64") != std::string::npos);
  CHECK(printModule(*module) == before);
  return 0;
}
