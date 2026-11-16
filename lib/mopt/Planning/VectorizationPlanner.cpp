#include "mopt/Planning/VectorizationPlanner.h"

#include "mopt/Analysis/LoopAnalysis.h"
#include "mopt/Target/TargetInfo.h"

#include "llvm/Support/ErrorHandling.h"

#include <algorithm>

namespace {

static unsigned largestPowerOfTwoAtMost(unsigned value) {
  unsigned result = 1;

  while (result <= value / 2)
    result *= 2;

  return result;
}

static mopt::VectorizationReason
mapLegalityReason(mopt::LegalityReason reason) {
  switch (reason) {
  case mopt::LegalityReason::LoopCarriedDependence:
    return mopt::VectorizationReason::LoopCarriedDependence;
  case mopt::LegalityReason::UnknownMemoryDependence:
    return mopt::VectorizationReason::UnknownMemoryDependence;
  case mopt::LegalityReason::UnknownMemoryEffect:
    return mopt::VectorizationReason::UnknownMemoryEffect;
  case mopt::LegalityReason::FloatingPointReductionRequiresReassociation:
    return mopt::VectorizationReason::
        FloatingPointReductionRequiresReassociation;
  }

  llvm_unreachable("unknown legality reason");
}

} // namespace

namespace mopt {

VectorizationPlan planVectorization(const LoopInfo &loopInfo,
                                    const TargetInfo &targetInfo) {
  VectorizationPlan plan;
  plan.targetVectorBits = targetInfo.fixedVectorRegisterBits;

  for (const auto &access : loopInfo.memoryAccesses) {
    if (!access.elementBitWidth)
      continue;

    if (!plan.widestMemoryElementBits)
      plan.widestMemoryElementBits = *access.elementBitWidth;
    else
      plan.widestMemoryElementBits =
          std::max(*plan.widestMemoryElementBits, *access.elementBitWidth);
  }

  if (plan.targetVectorBits == 0) {
    plan.decision = VectorizationDecision::Unknown;
    plan.reasons.push_back(VectorizationReason::UnknownTargetVectorWidth);
  } else if (!plan.widestMemoryElementBits ||
             *plan.widestMemoryElementBits == 0) {
    plan.decision = VectorizationDecision::Unknown;
    plan.reasons.push_back(VectorizationReason::UnknownMemoryElementWidth);
  } else {
    const unsigned availableLanes =
        plan.targetVectorBits / *plan.widestMemoryElementBits;
    plan.naturalFactor = largestPowerOfTwoAtMost(std::max(1u, availableLanes));
    plan.vectorFactor = plan.naturalFactor;
    plan.reasons.push_back(VectorizationReason::TargetWidthAndElementType);

    if (loopInfo.tripCount && *loopInfo.tripCount >= 0 &&
        static_cast<uint64_t>(*loopInfo.tripCount) < *plan.vectorFactor) {
      const unsigned tripLimit = static_cast<unsigned>(*loopInfo.tripCount);
      plan.vectorFactor = largestPowerOfTwoAtMost(std::max(1u, tripLimit));
      plan.reasons.push_back(VectorizationReason::TripCountLimitsFactor);
    }
  }

  for (LegalityReason reason : loopInfo.legality.reasons)
    plan.reasons.push_back(mapLegalityReason(reason));

  switch (loopInfo.legality.vectorization) {
  case LegalityState::Illegal:
    plan.decision = VectorizationDecision::Rejected;
    break;

  case LegalityState::Unknown:
    plan.decision = VectorizationDecision::Unknown;
    break;

  case LegalityState::Legal:
    plan.reasons.push_back(VectorizationReason::LegalityAllowsVectorization);

    if (!plan.vectorFactor) {
      plan.decision = VectorizationDecision::Unknown;
    } else if (*plan.vectorFactor <= 1) {
      plan.decision = VectorizationDecision::Rejected;
      plan.reasons.push_back(VectorizationReason::VectorFactorIsOne);
    } else {
      plan.decision = VectorizationDecision::Candidate;
    }

    break;
  }

  return plan;
}

} // namespace mopt
