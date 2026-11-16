#include "mopt/Passes.h"
#include "mopt/Target/TargetInfo.h"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/Dialect/SCF/IR/SCF.h"

#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/DialectRegistry.h"
#include "mlir/IR/MLIRContext.h"

#include "mlir/Parser/Parser.h"
#include "mlir/Pass/PassManager.h"

#include "llvm/Support/CommandLine.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"

using namespace mlir;

static llvm::cl::opt<std::string>
    inputFilename(llvm::cl::Positional, llvm::cl::desc("<input MLIR file>"),
                  llvm::cl::Required);

static llvm::cl::opt<bool>
    dumpIR("dump-ir",
           llvm::cl::desc("Dump the MLIR module after the pass pipeline"),
           llvm::cl::init(false));

static llvm::cl::opt<bool>
    dumpTargetInfo("dump-target-info",
                   llvm::cl::desc("Dump LLVM target-machine information"),
                   llvm::cl::init(false));

static llvm::cl::opt<std::string>
    targetTriple("target-triple",
                 llvm::cl::desc("LLVM target triple (defaults to the host)"),
                 llvm::cl::init(""));

static llvm::cl::opt<std::string>
    targetCPU("mcpu",
              llvm::cl::desc("LLVM target CPU (defaults to the host CPU)"),
              llvm::cl::init(""));

static llvm::cl::opt<std::string>
    targetFeatures("mattr", llvm::cl::desc("LLVM target feature string"),
                   llvm::cl::init(""));

int main(int argc, char **argv) {
  llvm::InitLLVM initLLVM(argc, argv);

  llvm::cl::ParseCommandLineOptions(argc, argv,
                                    "mopt - MLIR optimization tool\n");

  auto targetInfo =
      mopt::queryTargetInfo({targetTriple, targetCPU, targetFeatures});
  if (!targetInfo) {
    llvm::errs() << "mopt: failed to query LLVM target information: "
                 << llvm::toString(targetInfo.takeError()) << "\n";
    return 1;
  }

  if (dumpTargetInfo)
    targetInfo->print(llvm::outs());

  DialectRegistry registry;

  registry.insert<arith::ArithDialect, func::FuncDialect, memref::MemRefDialect,
                  scf::SCFDialect>();

  MLIRContext context(registry);

  OwningOpRef<ModuleOp> module =
      parseSourceFile<ModuleOp>(inputFilename, &context);

  if (!module) {
    llvm::errs() << "mopt: failed to parse " << inputFilename << "\n";

    return 1;
  }

  PassManager pm(&context);

  pm.addPass(mopt::createLoopAnalysisPass(*targetInfo, llvm::outs()));

  if (failed(pm.run(*module))) {
    llvm::errs() << "mopt: pass pipeline failed\n";

    return 1;
  }

  if (dumpIR) {
    llvm::outs() << "// ----- mopt IR after pipeline -----\n";
    module->print(llvm::outs());
    llvm::outs() << "\n";
  }

  return 0;
}
