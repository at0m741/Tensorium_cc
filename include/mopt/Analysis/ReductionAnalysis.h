#pragma once

#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/IR/Operation.h"
#include "mlir/IR/Value.h"
#include "llvm/ADT/SmallVector.h"
namespace mopt {
enum class ReductionKind { Add, Mul };

struct ReductionInfo {
  ReductionKind kind;
  unsigned iterArgIndex;

  mlir::Value accumulator;
  mlir::Value contribution;
  mlir::Operation *operation;

  bool floatingPoint;
  bool reassociationAllowed;
};

llvm::SmallVector<ReductionInfo> analyzeReductions(mlir::scf::ForOp loop);
} // namespace mopt
