#include "cc1/Driver/Toolchain.hpp"
#include "cc1/Driver/DriverConfig.hpp"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Program.h"
#include <ostream>

namespace cc1::driver {
std::optional<Toolchain> discoverToolchain(const DriverOptions &options,
                                           const std::string &executable,
                                           std::ostream &errors) {
  auto resolve = [&](const std::string &overridePath, const char *name,
                     const std::string &preferred) -> std::string {
    if (!overridePath.empty()) {
      auto program = llvm::sys::findProgramByName(overridePath);
      if (program && llvm::sys::fs::is_regular_file(*program) &&
          llvm::sys::fs::can_execute(*program))
        return *program;
      errors << "cc1: error: cannot execute tool '" << overridePath
             << "'\n";
      return {};
    }
    if (!preferred.empty() && llvm::sys::fs::is_regular_file(preferred) &&
        llvm::sys::fs::can_execute(preferred))
      return preferred;
    auto program = llvm::sys::findProgramByName(name);
    if (program)
      return *program;
    errors << "cc1: error: cannot find '" << name
           << "'; select its path with --" << name << "=PATH\n";
    return {};
  };
  auto sibling = [&](const char *name) {
    llvm::SmallString<256> path(executable);
    llvm::sys::path::remove_filename(path);
    llvm::sys::path::append(path, name);
    return std::string(path);
  };
  auto configured = [&](const char *name) {
    llvm::SmallString<256> path(TENSORIUM_LLVM_TOOLS_DIR);
    if (path.empty())
      return std::string{};
    llvm::sys::path::append(path, name);
    return std::string(path);
  };
  Toolchain tools;
  bool hasSources = false;
  for (const auto &input : options.inputs)
    hasSources |= input.kind == InputKind::Source;
  if (hasSources) {
    tools.cc1 = resolve(options.cc1, "cc1", sibling("cc1"));
    tools.optimizer =
        resolve(options.optimizer, "tensorium-opt", sibling("tensorium-opt"));
    tools.translator = resolve(options.translator, "mlir-translate",
                               configured("mlir-translate"));
    if (tools.cc1.empty() || tools.optimizer.empty() ||
        tools.translator.empty())
      return {};
  }
  tools.clang = resolve(options.clang, "clang", TENSORIUM_DRIVER_CLANG);
  if (tools.clang.empty())
    return {};
  return tools;
}
} // namespace cc1::driver
