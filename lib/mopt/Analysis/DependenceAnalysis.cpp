#include "mopt/Analysis/DependenceAnalysis.h"

#include "mlir/Dialect/Arith/IR/Arith.h"

#include <optional>

namespace mopt {
namespace {

enum class DistanceState { None, Known, Unknown };

struct DistanceResult {
  DistanceState state = DistanceState::Unknown;
  std::optional<int64_t> distance;
};

static DependenceKind classifyDependence(MemoryAccessKind source,
                                         MemoryAccessKind target) {
  if (source == MemoryAccessKind::Store && target == MemoryAccessKind::Load)
    return DependenceKind::RAW;

  if (source == MemoryAccessKind::Load && target == MemoryAccessKind::Store)
    return DependenceKind::WAR;

  if (source == MemoryAccessKind::Store && target == MemoryAccessKind::Store)
    return DependenceKind::WAW;

  return DependenceKind::Unknown;
}

static DistanceResult computeDistance(const MemoryAccessInfo &source,
                                      const MemoryAccessInfo &target,
                                      std::optional<int64_t> loopStep) {
  if (source.rank != target.rank)
    return {};

  if (source.indices.size() != target.indices.size())
    return {};

  std::optional<int64_t> distance;

  for (unsigned d = 0; d < source.indices.size(); ++d) {
    const auto &src = source.indices[d];
    const auto &dst = target.indices[d];

    if (src.valid && dst.valid) {
      if (src.coefficient != dst.coefficient)
        return {};

      if (src.coefficient == 0) {
        if (src.offset != dst.offset)
          return {DistanceState::None, std::nullopt};

        continue;
      }

      const int64_t numerator = src.offset - dst.offset;

      if (numerator % src.coefficient != 0)
        return {DistanceState::None, std::nullopt};

      // Index expressions are functions of the induction-variable value.
      // Convert that value distance to a number of loop iterations.
      const int64_t inductionDistance = numerator / src.coefficient;

      if (!loopStep || *loopStep <= 0) {
        if (inductionDistance != 0)
          return {};
      } else if (inductionDistance % *loopStep != 0) {
        return {DistanceState::None, std::nullopt};
      }

      const int64_t candidate =
          loopStep && *loopStep > 0 ? inductionDistance / *loopStep : 0;

      if (distance && *distance != candidate)
        return {DistanceState::None, std::nullopt};

      distance = candidate;
      continue;
    }

    if (src.invariant && dst.invariant && d < source.indexValues.size() &&
        d < target.indexValues.size() &&
        source.indexValues[d] == target.indexValues[d])
      continue;

    return {};
  }

  if (!distance)
    return {DistanceState::Known, 1};

  return {DistanceState::Known, distance};
}

static void recordDependence(unsigned sourceIndex, unsigned targetIndex,
                             const std::vector<MemoryAccessInfo> &accesses,
                             std::optional<int64_t> loopStep,
                             std::vector<DependenceInfo> &dependences) {
  const auto &source = accesses[sourceIndex];
  const auto &target = accesses[targetIndex];

  if (source.kind == MemoryAccessKind::Load &&
      target.kind == MemoryAccessKind::Load)
    return;

  const auto result = computeDistance(source, target, loopStep);

  if (result.state == DistanceState::None)
    return;

  if (result.state == DistanceState::Unknown) {
    dependences.push_back({
        sourceIndex,
        targetIndex,
        DependenceKind::Unknown,
        DependenceCertainty::May,
        true,
        std::nullopt,
    });

    return;
  }

  if (!result.distance || *result.distance <= 0)
    return;

  dependences.push_back({
      sourceIndex,
      targetIndex,
      classifyDependence(source.kind, target.kind),
      DependenceCertainty::Proven,
      true,
      result.distance,
  });
}

} // namespace

std::vector<DependenceInfo>
analyzeDependences(mlir::scf::ForOp loop,
                   const std::vector<MemoryAccessInfo> &accesses) {
  std::vector<DependenceInfo> dependences;

  std::optional<int64_t> loopStep;

  if (auto constant =
          loop.getStep().getDefiningOp<mlir::arith::ConstantIndexOp>())
    loopStep = constant.value();

  for (unsigned i = 0; i < accesses.size(); ++i) {
    // An invariant store conflicts with itself in different iterations.
    if (accesses[i].kind == MemoryAccessKind::Store)
      recordDependence(i, i, accesses, loopStep, dependences);

    for (unsigned j = i + 1; j < accesses.size(); ++j) {
      const auto &lhs = accesses[i];
      const auto &rhs = accesses[j];

      if (lhs.kind == MemoryAccessKind::Load &&
          rhs.kind == MemoryAccessKind::Load)
        continue;

      if (lhs.memref != rhs.memref) {
        dependences.push_back({
            i,
            j,
            DependenceKind::Unknown,
            DependenceCertainty::May,
            true,
            std::nullopt,
        });

        continue;
      }

      recordDependence(i, j, accesses, loopStep, dependences);
      recordDependence(j, i, accesses, loopStep, dependences);
    }
  }

  return dependences;
}

} // namespace mopt
