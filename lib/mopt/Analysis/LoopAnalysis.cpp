#include "mopt/Analysis/LoopAnalysis.h"
#include "mopt/Analysis/DependenceAnalysis.h"
#include "mopt/Analysis/LegalityAnalysis.h"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/IR/Operation.h"
#include "mlir/IR/Value.h"
#include "mlir/Interfaces/SideEffectInterfaces.h"
#include "mopt/Analysis/ReductionAnalysis.h"

#include <cstdint>
#include <optional>

using namespace mlir;

namespace {

static std::optional<int64_t> getConstantIndex(Value value) {
  if (auto constant = value.getDefiningOp<arith::ConstantIndexOp>()) {
    return constant.value();
  }

  return std::nullopt;
}

static bool dependsOnIV(Value value, Value iv) {
  if (value == iv)
    return true;

  Operation *op = value.getDefiningOp();

  if (!op)
    return false;

  for (Value operand : op->getOperands()) {
    if (dependsOnIV(operand, iv))
      return true;
  }

  return false;
}

static mopt::LinearIndexExpr analyzeIndexExpr(Value value, Value iv) {
  if (value == iv) {
    return {1, 0, true, false};
  }

  if (auto constant = value.getDefiningOp<arith::ConstantIndexOp>()) {
    return {0, constant.value(), true, true};
  }

  if (!dependsOnIV(value, iv)) {
    return {0, 0, false, true};
  }

  if (auto add = value.getDefiningOp<arith::AddIOp>()) {
    auto lhs = analyzeIndexExpr(add.getLhs(), iv);
    auto rhs = analyzeIndexExpr(add.getRhs(), iv);

    if (!lhs.valid || !rhs.valid)
      return {};

    return {lhs.coefficient + rhs.coefficient, lhs.offset + rhs.offset, true,
            false};
  }

  if (auto sub = value.getDefiningOp<arith::SubIOp>()) {
    auto lhs = analyzeIndexExpr(sub.getLhs(), iv);
    auto rhs = analyzeIndexExpr(sub.getRhs(), iv);

    if (!lhs.valid || !rhs.valid)
      return {};

    return {lhs.coefficient - rhs.coefficient, lhs.offset - rhs.offset, true,
            false};
  }

  if (auto mul = value.getDefiningOp<arith::MulIOp>()) {
    auto lhs = analyzeIndexExpr(mul.getLhs(), iv);
    auto rhs = analyzeIndexExpr(mul.getRhs(), iv);

    if (!lhs.valid || !rhs.valid)
      return {};

    if (lhs.coefficient != 0 && rhs.coefficient != 0)
      return {};

    return {lhs.coefficient * rhs.offset + rhs.coefficient * lhs.offset,
            lhs.offset * rhs.offset, true, false};
  }

  return {};
}

static mopt::MemoryAccessInfo
analyzeMemoryAccess(mopt::MemoryAccessKind kind, Value memref,
                    ValueRange indices, Value iv,
                    std::optional<int64_t> loopStep) {
  mopt::MemoryAccessInfo info;

  info.kind = kind;
  info.rank = static_cast<unsigned>(indices.size());
  info.memref = memref;

  auto memrefType = dyn_cast<MemRefType>(memref.getType());
  if (memrefType) {
    Type elementType = memrefType.getElementType();

    if (auto integerType = dyn_cast<IntegerType>(elementType))
      info.elementBitWidth = integerType.getWidth();
    else if (auto floatType = dyn_cast<FloatType>(elementType))
      info.elementBitWidth = floatType.getWidth();
  }

  for (Value index : indices) {
    info.indexValues.push_back(index);
    auto expr = analyzeIndexExpr(index, iv);
    info.indices.push_back(expr);

    if (expr.valid && expr.coefficient != 0) {
      info.usesInductionVariable = true;
    }

    if (index == iv) {
      info.directInductionVariable = true;
    }
  }

  if (!loopStep)
    return info;

  for (const auto &expr : info.indices) {
    if (expr.valid) {
      info.dimensionStrides.push_back(expr.coefficient * *loopStep);
    } else if (expr.invariant) {
      info.dimensionStrides.push_back(0);
    } else {
      info.dimensionStrides.push_back(std::nullopt);
    }
  }

  if (!memrefType)
    return info;

  auto [strides, offset] = memrefType.getStridesAndOffset();
  (void)offset;

  if (strides.size() != info.dimensionStrides.size()) {
    return info;
  }

  int64_t physicalStride = 0;
  for (unsigned d = 0; d < info.dimensionStrides.size(); ++d) {
    if (!info.dimensionStrides[d])
      return info;

    if (ShapedType::isDynamic(strides[d])) {
      return info;
    }

    physicalStride += *info.dimensionStrides[d] * strides[d];
  }

  info.physicalStride = physicalStride;
  info.unitStride = physicalStride == 1 || physicalStride == -1;

  return info;
}

static unsigned getLoopDepth(scf::ForOp loop) {
  unsigned depth = 0;

  Operation *parent = loop->getParentOp();

  while (parent) {
    if (isa<scf::ForOp>(parent))
      ++depth;

    parent = parent->getParentOp();
  }

  return depth;
}

static bool hasUnknownMemoryEffects(Operation *op) {
  if (auto effects = dyn_cast<MemoryEffectOpInterface>(op)) {
    SmallVector<MemoryEffects::EffectInstance> instances;
    effects.getEffects(instances);
    return !instances.empty();
  }

  return !op->hasTrait<OpTrait::HasRecursiveMemoryEffects>();
}

} // namespace

mopt::LoopInfo mopt::analyzeLoop(scf::ForOp loop) {
  mopt::LoopInfo info;

  info.depth = getLoopDepth(loop);

  auto parentLoop = loop->getParentOfType<scf::ForOp>();

  info.hasParent = static_cast<bool>(parentLoop);
  info.lowerBound = getConstantIndex(loop.getLowerBound());
  info.upperBound = getConstantIndex(loop.getUpperBound());
  info.step = getConstantIndex(loop.getStep());

  if (info.lowerBound && info.upperBound && info.step) {

    const int64_t lb = *info.lowerBound;
    const int64_t ub = *info.upperBound;
    const int64_t step = *info.step;

    if (step > 0) {
      if (ub <= lb) {
        info.tripCount = 0;
      } else {
        const int64_t distance = ub - lb;

        info.tripCount = (distance + step - 1) / step;
      }
    }
  }

  Value iv = loop.getInductionVar();

  loop.walk([&](Operation *op) {
    if (op == loop.getOperation())
      return;

    if (auto load = dyn_cast<memref::LoadOp>(op)) {
      ++info.loads;

      auto access =
          analyzeMemoryAccess(mopt::MemoryAccessKind::Load, load.getMemRef(),
                              load.getIndices(), iv, info.step);

      if (access.directInductionVariable)
        ++info.directIVLoads;

      info.memoryAccesses.push_back(std::move(access));
      return;
    }

    if (auto store = dyn_cast<memref::StoreOp>(op)) {
      ++info.stores;

      auto access =
          analyzeMemoryAccess(mopt::MemoryAccessKind::Store, store.getMemRef(),
                              store.getIndices(), iv, info.step);

      if (access.directInductionVariable)
        ++info.directIVStores;

      info.memoryAccesses.push_back(std::move(access));
      return;
    }

    if (op->getName().getDialectNamespace() == "arith")
      ++info.arithmeticOps;

    if (hasUnknownMemoryEffects(op))
      info.hasUnknownMemoryEffects = true;
  });

  auto reductions = mopt::analyzeReductions(loop);
  info.reductions.assign(reductions.begin(), reductions.end());

  info.dependences = mopt::analyzeDependences(loop, info.memoryAccesses);
  info.legality = mopt::analyzeLegality(info);

  return info;
}
