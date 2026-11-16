#include "mopt/Analysis/LegalityAnalysis.h"

namespace mopt {

static void addReason(LegalityInfo &info, LegalityReason reason) {
  for (LegalityReason current : info.reasons) {
    if (current == reason)
      return;
  }

  info.reasons.push_back(reason);
}

LegalityInfo analyzeLegality(const LoopInfo &info) {
  LegalityInfo result;

  if (info.hasUnknownMemoryEffects) {
    result.vectorization = LegalityState::Unknown;
    result.parallelization = LegalityState::Unknown;
    addReason(result, LegalityReason::UnknownMemoryEffect);
  }

  for (const auto &dependence : info.dependences) {
    if (dependence.certainty == DependenceCertainty::Proven) {
      result.vectorization = LegalityState::Illegal;
      result.parallelization = LegalityState::Illegal;
      addReason(result, LegalityReason::LoopCarriedDependence);
      continue;
    }

    if (result.vectorization != LegalityState::Illegal)
      result.vectorization = LegalityState::Unknown;

    if (result.parallelization != LegalityState::Illegal)
      result.parallelization = LegalityState::Unknown;

    addReason(result, LegalityReason::UnknownMemoryDependence);
  }

  for (const auto &reduction : info.reductions) {
    if (reduction.floatingPoint && !reduction.reassociationAllowed) {
      result.vectorization = LegalityState::Illegal;
      result.parallelization = LegalityState::Illegal;

      addReason(result,
                LegalityReason::FloatingPointReductionRequiresReassociation);
    }
  }

  return result;
}

} // namespace mopt
