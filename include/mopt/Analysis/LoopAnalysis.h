#pragma once

#include "mopt/Analysis/ReductionAnalysis.h"

#include <cstdint>
#include <optional>
#include <vector>

namespace mopt {

enum class MemoryAccessKind { Load, Store };

enum class DependenceKind { RAW, WAR, WAW, Unknown };

enum class DependenceCertainty { Proven, May };

struct DependenceInfo {
  unsigned sourceAccess = 0;
  unsigned targetAccess = 0;
  DependenceKind kind = DependenceKind::Unknown;
  DependenceCertainty certainty = DependenceCertainty::May;
  bool loopCarried = false;
  std::optional<int64_t> distance;
};

struct LinearIndexExpr {
  int64_t coefficient = 0;
  int64_t offset = 0;
  bool valid = false;
  bool invariant = false;
};

enum class LegalityState { Legal, Illegal, Unknown };

enum class LegalityReason {
  LoopCarriedDependence,
  UnknownMemoryDependence,
  UnknownMemoryEffect,
  FloatingPointReductionRequiresReassociation
};

struct LegalityInfo {
  LegalityState vectorization = LegalityState::Legal;
  LegalityState parallelization = LegalityState::Legal;
  std::vector<LegalityReason> reasons;
};

struct MemoryAccessInfo {
  MemoryAccessKind kind;

  unsigned rank = 0;
  mlir::Value memref;
  std::vector<mlir::Value> indexValues;
  bool usesInductionVariable = false;
  bool directInductionVariable = false;
  bool unitStride = false;

  std::vector<LinearIndexExpr> indices;

  std::vector<std::optional<int64_t>> dimensionStrides;
  std::optional<int64_t> physicalStride;
  std::optional<unsigned> elementBitWidth;
};

struct LoopInfo {
  std::optional<int64_t> lowerBound;
  std::optional<int64_t> upperBound;
  std::optional<int64_t> step;
  std::optional<int64_t> tripCount;

  unsigned depth = 0;
  bool hasParent = false;

  unsigned loads = 0;
  unsigned stores = 0;
  unsigned arithmeticOps = 0;
  unsigned directIVLoads = 0;
  unsigned directIVStores = 0;

  std::vector<MemoryAccessInfo> memoryAccesses;
  std::vector<ReductionInfo> reductions;

  bool hasUnknownMemoryEffects = false;
  bool allMemoryAccessesUnitStride = false;
  std::vector<DependenceInfo> dependences;
  LegalityInfo legality;
};

// Analyze an SCF loop without mutating IR or producing a report.
// Returned MLIR values remain valid only while the source IR exists.
LoopInfo analyzeLoop(mlir::scf::ForOp loop);

} // namespace mopt
