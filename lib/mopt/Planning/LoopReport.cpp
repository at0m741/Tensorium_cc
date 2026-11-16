#include "mopt/Planning/LoopReport.h"
#include "mopt/Analysis/LoopAnalysis.h"
#include "mopt/Planning/VectorizationPlanner.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

namespace {

static llvm::StringRef dependenceKindName(mopt::DependenceKind kind) {
  switch (kind) {
  case mopt::DependenceKind::RAW:
    return "RAW";
  case mopt::DependenceKind::WAR:
    return "WAR";
  case mopt::DependenceKind::WAW:
    return "WAW";
  case mopt::DependenceKind::Unknown:
    return "unknown";
  }

  llvm_unreachable("unknown dependence kind");
}

static llvm::StringRef legalityStateName(mopt::LegalityState state) {
  switch (state) {
  case mopt::LegalityState::Legal:
    return "legal";
  case mopt::LegalityState::Illegal:
    return "illegal";
  case mopt::LegalityState::Unknown:
    return "unknown";
  }

  llvm_unreachable("unknown legality state");
}

static llvm::StringRef legalityReasonName(mopt::LegalityReason reason) {
  switch (reason) {
  case mopt::LegalityReason::LoopCarriedDependence:
    return "loop-carried-dependence";

  case mopt::LegalityReason::UnknownMemoryDependence:
    return "unknown-memory-dependence";

  case mopt::LegalityReason::UnknownMemoryEffect:
    return "unknown-memory-effect";

  case mopt::LegalityReason::FloatingPointReductionRequiresReassociation:
    return "floating-point-reduction-requires-reassociation";
  }

  llvm_unreachable("unknown legality reason");
}

static llvm::StringRef
vectorizationDecisionName(mopt::VectorizationDecision decision) {
  switch (decision) {
  case mopt::VectorizationDecision::Pending:
    return "pending";
  case mopt::VectorizationDecision::Candidate:
    return "candidate";
  case mopt::VectorizationDecision::Rejected:
    return "rejected";
  case mopt::VectorizationDecision::Unknown:
    return "unknown";
  }

  llvm_unreachable("unknown vectorization decision");
}

static llvm::StringRef
vectorTailStrategyName(mopt::VectorTailStrategy strategy) {
  switch (strategy) {
  case mopt::VectorTailStrategy::NotEvaluated:
    return "not-evaluated";
  case mopt::VectorTailStrategy::None:
    return "none";
  case mopt::VectorTailStrategy::ScalarEpilogue:
    return "scalar-epilogue";
  case mopt::VectorTailStrategy::Unknown:
    return "unknown";
  }

  llvm_unreachable("unknown vector tail strategy");
}

static llvm::StringRef
vectorizationReasonName(mopt::VectorizationReason reason) {
  switch (reason) {
  case mopt::VectorizationReason::TargetWidthAndElementType:
    return "target-width-and-element-type";
  case mopt::VectorizationReason::TripCountLimitsFactor:
    return "trip-count-limits-factor";
  case mopt::VectorizationReason::UnknownTargetVectorWidth:
    return "unknown-target-vector-width";
  case mopt::VectorizationReason::UnknownMemoryElementWidth:
    return "unknown-memory-element-width";
  case mopt::VectorizationReason::LegalityAllowsVectorization:
    return "legality-allows-vectorization";
  case mopt::VectorizationReason::VectorFactorIsOne:
    return "vector-factor-is-one";
  case mopt::VectorizationReason::LoopCarriedDependence:
    return "loop-carried-dependence";
  case mopt::VectorizationReason::UnknownMemoryDependence:
    return "unknown-memory-dependence";
  case mopt::VectorizationReason::UnknownMemoryEffect:
    return "unknown-memory-effect";
  case mopt::VectorizationReason::FloatingPointReductionRequiresReassociation:
    return "floating-point-reduction-requires-reassociation";
  }

  llvm_unreachable("unknown vectorization reason");
}

} // namespace

void mopt::printLoopReport(llvm::raw_ostream &os, unsigned loopId,
                           const LoopInfo &info,
                           const VectorizationPlan &plan) {
  os << "mopt.loop #" << loopId << "\n";

  os << "  topology:\n"
     << "    depth = " << info.depth << "\n"
     << "    parent = " << (info.hasParent ? "true" : "false") << "\n";

  os << "  iteration:\n"
     << "    lower = ";

  if (info.lowerBound)
    os << *info.lowerBound;
  else
    os << "dynamic";

  os << "\n"
     << "    upper = ";

  if (info.upperBound)
    os << *info.upperBound;
  else
    os << "dynamic";

  os << "\n"
     << "    step = ";

  if (info.step)
    os << *info.step;
  else
    os << "dynamic";

  os << "\n"
     << "    trip-count = ";

  if (info.tripCount) {
    os << *info.tripCount;
  } else if (info.step && *info.step <= 0) {
    os << "unsupported";
  } else {
    os << "dynamic";
  }

  os << "\n"
     << "  body:\n"
     << "    loads = " << info.loads << "\n"
     << "    stores = " << info.stores << "\n"
     << "    arith-ops = " << info.arithmeticOps << "\n"
     << "    direct-iv-loads = " << info.directIVLoads << "\n"
     << "    direct-iv-stores = " << info.directIVStores << "\n";

  os << "  reductions:\n"
     << "    count = " << info.reductions.size() << "\n";

  for (unsigned i = 0; i < info.reductions.size(); ++i) {
    const auto &reduction = info.reductions[i];

    os << "    reduction #" << i << "\n"
       << "      iter-arg = " << reduction.iterArgIndex << "\n"
       << "      kind = ";

    switch (reduction.kind) {
    case mopt::ReductionKind::Add:
      os << "add";
      break;
    case mopt::ReductionKind::Mul:
      os << "mul";
      break;
    }

    os << "\n"
       << "      floating-point = "
       << (reduction.floatingPoint ? "true" : "false") << "\n"
       << "      reassociation = "
       << (reduction.reassociationAllowed ? "true" : "false") << "\n";
  }

  os << "  memory:\n"
     << "    accesses = " << info.memoryAccesses.size() << "\n"
     << "    unknown-effects = "
     << (info.hasUnknownMemoryEffects ? "true" : "false") << "\n";

  for (unsigned i = 0; i < info.memoryAccesses.size(); ++i) {
    const auto &access = info.memoryAccesses[i];

    os << "    access #" << i << " "
       << (access.kind == mopt::MemoryAccessKind::Load ? "load" : "store")
       << " rank=" << access.rank << "\n"
       << "      uses-iv = "
       << (access.usesInductionVariable ? "true" : "false") << "\n"
       << "      direct-iv = "
       << (access.directInductionVariable ? "true" : "false") << "\n"
       << "      index-strides = [";

    for (unsigned d = 0; d < access.dimensionStrides.size(); ++d) {
      if (d != 0)
        os << ", ";

      if (access.dimensionStrides[d])
        os << *access.dimensionStrides[d];
      else
        os << "?";
    }

    os << "]\n"
       << "      physical-stride = ";

    if (access.physicalStride)
      os << *access.physicalStride;
    else
      os << "unknown";

    os << "\n"
       << "      unit-stride = " << (access.unitStride ? "true" : "false")
       << "\n";

    for (unsigned d = 0; d < access.indices.size(); ++d) {
      const auto &expr = access.indices[d];

      os << "      index[" << d << "] = ";

      if (expr.valid) {
        os << expr.coefficient << "*iv";

        if (expr.offset > 0)
          os << "+" << expr.offset;
        else if (expr.offset < 0)
          os << expr.offset;
      } else if (expr.invariant) {
        os << "invariant";
      } else {
        os << "unknown";
      }

      os << "\n";
    }
  }

  os << "  dependences:\n"
     << "    loop-carried = " << info.dependences.size() << "\n";

  for (unsigned i = 0; i < info.dependences.size(); ++i) {
    const auto &dependence = info.dependences[i];

    os << "    dependence #" << i << "\n"
       << "      source-access = " << dependence.sourceAccess << "\n"
       << "      target-access = " << dependence.targetAccess << "\n"
       << "      kind = " << dependenceKindName(dependence.kind) << "\n"
       << "      certainty = "
       << (dependence.certainty == mopt::DependenceCertainty::Proven ? "proven"
                                                                     : "may")
       << "\n"
       << "      distance = ";

    if (dependence.distance)
      os << *dependence.distance;
    else
      os << "unknown";

    os << "\n";
  }

  os << "  target:\n"
     << "    fixed-vector-bits = ";

  if (plan.targetVectorBits != 0)
    os << plan.targetVectorBits;
  else
    os << "unknown";

  os << "\n"
     << "    widest-memory-element-bits = ";

  if (plan.widestMemoryElementBits)
    os << *plan.widestMemoryElementBits;
  else
    os << "unknown";

  os << "\n"
     << "  vector-plan:\n"
     << "    decision = " << vectorizationDecisionName(plan.decision) << "\n"
     << "    natural-factor = ";

  if (plan.naturalFactor)
    os << *plan.naturalFactor;
  else
    os << "unknown";

  os << "\n"
     << "    vector-factor = ";

  if (plan.vectorFactor)
    os << *plan.vectorFactor;
  else
    os << "unknown";

  os << "\n"
     << "    interleave-factor = " << plan.interleaveFactor << "\n"
     << "    tail = " << vectorTailStrategyName(plan.tailStrategy) << "\n"
     << "    reasons = [";

  for (unsigned i = 0; i < plan.reasons.size(); ++i) {
    if (i != 0)
      os << ", ";

    os << vectorizationReasonName(plan.reasons[i]);
  }

  os << "]\n";

  os << "  legality:\n"
     << "    vectorization = " << legalityStateName(info.legality.vectorization)
     << "\n"
     << "    parallelization = "
     << legalityStateName(info.legality.parallelization) << "\n"
     << "    reasons = [";

  for (unsigned i = 0; i < info.legality.reasons.size(); ++i) {
    if (i != 0)
      os << ", ";

    os << legalityReasonName(info.legality.reasons[i]);
  }

  os << "]\n\n";
}
