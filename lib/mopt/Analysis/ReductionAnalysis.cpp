#include "mopt/Analysis/ReductionAnalysis.h"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"

using namespace mlir;

namespace mopt {
using namespace mlir;
namespace {

static bool dependsOnImpl(Value value, Value target,
                          llvm::SmallPtrSetImpl<Operation *> &visited) {
  if (value == target)
    return true;

  Operation *def = value.getDefiningOp();
  if (!def)
    return false;

  if (!visited.insert(def).second)
    return false;

  for (Value operand : def->getOperands()) {
    if (dependsOnImpl(operand, target, visited))
      return true;
  }

  return false;
}

static bool dependsOn(Value value, Value target) {
  llvm::SmallPtrSet<Operation *, 16> visited;
  return dependsOnImpl(value, target, visited);
}

static bool hasReassoc(Operation *op) {
  auto attr = op->getAttrOfType<arith::FastMathFlagsAttr>("fastmath");
  if (!attr)
    return false;

  auto flags = static_cast<uint32_t>(attr.getValue());
  auto reassoc = static_cast<uint32_t>(arith::FastMathFlags::reassoc);

  return (flags & reassoc) != 0;
}

static std::optional<ReductionInfo>
matchBinaryReduction(Value accumulator, Value yielded, unsigned iterArgIndex) {
  Operation *op = yielded.getDefiningOp();
  if (!op || op->getNumOperands() != 2 || op->getNumResults() != 1)
    return std::nullopt;

  Value lhs = op->getOperand(0);
  Value rhs = op->getOperand(1);

  Value contribution;

  if (lhs == accumulator)
    contribution = rhs;
  else if (rhs == accumulator)
    contribution = lhs;
  else
    return std::nullopt;
  if (dependsOn(contribution, accumulator))
    return std::nullopt;

  ReductionInfo info;
  info.iterArgIndex = iterArgIndex;
  info.accumulator = accumulator;
  info.contribution = contribution;
  info.operation = op;
  info.reassociationAllowed = false;

  if (isa<arith::AddIOp>(op)) {
    info.kind = ReductionKind::Add;
    info.floatingPoint = false;
    info.reassociationAllowed = true;
    return info;
  }

  if (isa<arith::MulIOp>(op)) {
    info.kind = ReductionKind::Mul;
    info.floatingPoint = false;
    info.reassociationAllowed = true;
    return info;
  }

  if (isa<arith::AddFOp>(op)) {
    info.kind = ReductionKind::Add;
    info.floatingPoint = true;
    info.reassociationAllowed = hasReassoc(op);
    return info;
  }

  if (isa<arith::MulFOp>(op)) {
    info.kind = ReductionKind::Mul;
    info.floatingPoint = true;
    info.reassociationAllowed = hasReassoc(op);
    return info;
  }

  return std::nullopt;
}

} // namespace

llvm::SmallVector<ReductionInfo> analyzeReductions(scf::ForOp loop) {
  llvm::SmallVector<ReductionInfo> reductions;

  auto iterArgs = loop.getRegionIterArgs();
  if (iterArgs.empty())
    return reductions;
  auto yield = dyn_cast<scf::YieldOp>(loop.getBody()->getTerminator());
  if (!yield)
    return reductions;

  auto yieldedValues = yield.getResults();
  if (yieldedValues.size() != iterArgs.size())
    return reductions;

  for (unsigned i = 0; i < iterArgs.size(); ++i) {
    auto reduction = matchBinaryReduction(iterArgs[i], yieldedValues[i], i);
    if (reduction)
      reductions.push_back(*reduction);
  }

  return reductions;
}

} // namespace mopt
