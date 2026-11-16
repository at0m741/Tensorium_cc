#pragma once

#include <optional>
#include <vector>

namespace mopt {

struct LoopInfo;
struct TargetInfo;

enum class VectorizationDecision { Pending, Candidate, Rejected, Unknown };

enum class VectorTailStrategy { NotEvaluated, None, ScalarEpilogue, Unknown };

enum class VectorizationReason {
  TargetWidthAndElementType,
  TripCountLimitsFactor,
  UnknownTargetVectorWidth,
  UnknownMemoryElementWidth,
  LegalityAllowsVectorization,
  VectorFactorIsOne,
  LoopCarriedDependence,
  UnknownMemoryDependence,
  UnknownMemoryEffect,
  FloatingPointReductionRequiresReassociation
};

struct VectorizationPlan {
  VectorizationDecision decision = VectorizationDecision::Pending;
  VectorTailStrategy tailStrategy = VectorTailStrategy::NotEvaluated;

  unsigned targetVectorBits = 0;
  std::optional<unsigned> widestMemoryElementBits;
  std::optional<unsigned> naturalFactor;
  std::optional<unsigned> vectorFactor;
  unsigned interleaveFactor = 1;

  std::vector<VectorizationReason> reasons;
};

VectorizationPlan planVectorization(const LoopInfo &loopInfo,
                                    const TargetInfo &targetInfo);

} // namespace mopt
