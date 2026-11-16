#pragma once

#include "llvm/Support/Error.h"

#include <optional>
#include <string>

namespace llvm {
class raw_ostream;
}

namespace mopt {

struct TargetConfig {
  std::string triple;
  std::string cpu;
  std::string features;
};

struct TargetInfo {
  std::string triple;
  std::string cpu;
  std::string features;
  std::string dataLayout;

  unsigned pointerBits = 0;
  unsigned scalarRegisterBits = 0;
  unsigned fixedVectorRegisterBits = 0;
  unsigned scalableVectorRegisterMinBits = 0;
  unsigned loadStoreVectorRegisterBits = 0;
  unsigned cacheLineBytes = 0;
  std::optional<unsigned> l1DataCacheBytes;
  std::optional<unsigned> l2DataCacheBytes;

  bool littleEndian = true;

  void print(llvm::raw_ostream &os) const;
};

llvm::Expected<TargetInfo> queryTargetInfo(TargetConfig config);

} // namespace mopt
