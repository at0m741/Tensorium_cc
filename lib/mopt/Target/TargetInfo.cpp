#include "mopt/Target/TargetInfo.h"

#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/Config/llvm-config.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/TargetParser/Host.h"
#include "llvm/TargetParser/Triple.h"

#include <memory>
#include <system_error>

namespace {

static void printUnsignedOrUnknown(llvm::raw_ostream &os, unsigned value) {
  if (value == 0)
    os << "unknown";
  else
    os << value;
}

static void printOptionalOrUnknown(llvm::raw_ostream &os,
                                   std::optional<unsigned> value) {
  if (value)
    os << *value;
  else
    os << "unknown";
}

} // namespace

namespace mopt {

void TargetInfo::print(llvm::raw_ostream &os) const {
  os << "mopt.target\n"
     << "  configuration:\n"
     << "    triple = " << triple << "\n"
     << "    cpu = " << cpu << "\n"
     << "    features = " << (features.empty() ? "none" : features) << "\n"
     << "  data-layout:\n"
     << "    pointer-bits = " << pointerBits << "\n"
     << "    endianness = " << (littleEndian ? "little" : "big") << "\n"
     << "    layout = " << dataLayout << "\n"
     << "  registers:\n"
     << "    scalar-bits = ";
  printUnsignedOrUnknown(os, scalarRegisterBits);
  os << "\n"
     << "    fixed-vector-bits = ";
  printUnsignedOrUnknown(os, fixedVectorRegisterBits);
  os << "\n"
     << "    scalable-vector-min-bits = ";
  printUnsignedOrUnknown(os, scalableVectorRegisterMinBits);
  os << "\n"
     << "    load-store-vector-bits = ";
  printUnsignedOrUnknown(os, loadStoreVectorRegisterBits);
  os << "\n"
     << "  cache:\n"
     << "    line-bytes = ";
  printUnsignedOrUnknown(os, cacheLineBytes);
  os << "\n"
     << "    l1d-bytes = ";
  printOptionalOrUnknown(os, l1DataCacheBytes);
  os << "\n"
     << "    l2d-bytes = ";
  printOptionalOrUnknown(os, l2DataCacheBytes);
  os << "\n\n";
}

llvm::Expected<TargetInfo> queryTargetInfo(TargetConfig config) {
  if (llvm::InitializeNativeTarget())
    return llvm::createStringError(std::errc::not_supported,
                                   "LLVM native target initialization failed");

  if (config.triple.empty())
    config.triple = llvm::sys::getDefaultTargetTriple();

  config.triple = llvm::Triple::normalize(config.triple);

  const llvm::Triple requestedTriple(config.triple);
  const llvm::Triple processTriple(llvm::sys::getProcessTriple());

  if (config.cpu.empty()) {
    if (requestedTriple.getArch() == processTriple.getArch()) {
      const llvm::StringRef hostCPU = llvm::sys::getHostCPUName();
      config.cpu = hostCPU.empty() ? "generic" : hostCPU.str();
    } else {
      config.cpu = "generic";
    }
  }

  std::string lookupError;
#if LLVM_VERSION_MAJOR >= 24
  const auto &targetTriple = requestedTriple;
#else
  const auto &targetTriple = config.triple;
#endif
  const llvm::Target *target =
      llvm::TargetRegistry::lookupTarget(targetTriple, lookupError);
  if (!target)
    return llvm::createStringError(std::errc::invalid_argument, "%s",
                                   lookupError.c_str());

  llvm::TargetOptions options;
  std::unique_ptr<llvm::TargetMachine> targetMachine(
      target->createTargetMachine(targetTriple, config.cpu, config.features,
                                  options, std::nullopt));
  if (!targetMachine)
    return llvm::createStringError(
        std::errc::not_supported,
        "LLVM could not create a TargetMachine for '%s' with CPU '%s'",
        config.triple.c_str(), config.cpu.c_str());

  const llvm::DataLayout dataLayout = targetMachine->createDataLayout();

  llvm::LLVMContext context;
  llvm::Module probeModule("mopt.target-probe", context);
  probeModule.setTargetTriple(targetTriple);
  probeModule.setDataLayout(dataLayout);

  llvm::Function *probeFunction = llvm::Function::Create(
      llvm::FunctionType::get(llvm::Type::getVoidTy(context), false),
      llvm::GlobalValue::ExternalLinkage, "mopt.target.probe", probeModule);

  const llvm::TargetTransformInfo transformInfo =
      targetMachine->getTargetTransformInfo(*probeFunction);

  TargetInfo info;
  info.triple = config.triple;
  info.cpu = config.cpu;
  info.features = config.features;
  info.dataLayout = dataLayout.getStringRepresentation();
  info.pointerBits = dataLayout.getPointerSizeInBits();
  info.littleEndian = dataLayout.isLittleEndian();
  info.scalarRegisterBits =
      transformInfo.getRegisterBitWidth(llvm::TargetTransformInfo::RGK_Scalar)
          .getKnownMinValue();
  info.fixedVectorRegisterBits =
      transformInfo
          .getRegisterBitWidth(llvm::TargetTransformInfo::RGK_FixedWidthVector)
          .getKnownMinValue();
  info.scalableVectorRegisterMinBits =
      transformInfo
          .getRegisterBitWidth(llvm::TargetTransformInfo::RGK_ScalableVector)
          .getKnownMinValue();
  info.loadStoreVectorRegisterBits =
      transformInfo.getLoadStoreVecRegBitWidth(0);
  info.cacheLineBytes = transformInfo.getCacheLineSize();
  info.l1DataCacheBytes =
      transformInfo.getCacheSize(llvm::TargetTransformInfo::CacheLevel::L1D);
  info.l2DataCacheBytes =
      transformInfo.getCacheSize(llvm::TargetTransformInfo::CacheLevel::L2D);

  return info;
}

} // namespace mopt
