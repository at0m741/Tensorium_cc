#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlowOps.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/Tools/mlir-opt/MlirOptMain.h"
#include "mlir/Transforms/Passes.h"
#include "tensorium/Conversion/TensoriumToMemRef/TensoriumToMemRef.h"
#include "tensorium/Dialect/Tensorium/IR/TensoriumDialect.h"
#ifdef TENSORIUM_ENABLE_MOPT
#include "mopt/Passes.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#endif

int main(int argc, char **argv) {
  tensorium::registerTensoriumToMemRefPass();
#ifdef TENSORIUM_ENABLE_MOPT
  mopt::registerMOptPasses();
#endif
  mlir::registerTransformsPasses();
  mlir::DialectRegistry registry;
  registry.insert<tensorium::TensoriumDialect, mlir::arith::ArithDialect,
                  mlir::cf::ControlFlowDialect, mlir::func::FuncDialect,
                  mlir::memref::MemRefDialect>();
#ifdef TENSORIUM_ENABLE_MOPT
  registry.insert<mlir::scf::SCFDialect>();
#endif
  return mlir::asMainReturnCode(
      mlir::MlirOptMain(argc, argv, "Tensorium optimizer\n", registry));
}
