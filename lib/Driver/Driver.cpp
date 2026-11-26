#include "cc1/Driver/Driver.hpp"
#include "cc1/Driver/DriverConfig.hpp"
#include "cc1/Driver/DriverOptions.hpp"
#include "cc1/Driver/Toolchain.hpp"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Program.h"
#include <iomanip>
#include <iostream>
#include <optional>
#include <set>
#include <vector>

namespace cc1::driver {
namespace {
int executableAnchor;

int error(const std::string &message) {
  std::cerr << "cc1: error: " << message << '\n';
  return 1;
}

std::string absolute(const std::string &path) {
  llvm::SmallString<256> result(path);
  if (auto ec = llvm::sys::fs::make_absolute(result)) {
    error("cannot resolve '" + path + "': " + ec.message());
    return {};
  }
  // Preserve '..': lexical simplification changes paths through symlinks.
  return std::string(result);
}

class Workspace {
public:
  Workspace() = default;
  Workspace(const Workspace &) = delete;
  Workspace &operator=(const Workspace &) = delete;
  llvm::SmallString<256> directory;
  bool create() {
    llvm::SmallString<256> prefix;
    llvm::sys::path::system_temp_directory(true, prefix);
    llvm::sys::path::append(prefix, "tensoriumcc");
    if (auto ec = llvm::sys::fs::createUniqueDirectory(prefix, directory)) {
      error("cannot create temporary directory: " + ec.message());
      return false;
    }
    return true;
  }
  ~Workspace() {
    if (!directory.empty())
      llvm::sys::fs::remove_directories(directory);
  }
  std::string file(size_t index, const char *suffix) const {
    llvm::SmallString<256> path(directory);
    llvm::sys::path::append(path, std::to_string(index) + suffix);
    return std::string(path);
  }
};

// Stage beside the destination so publishing is a same-filesystem rename.
class OutputFile {
public:
  OutputFile() = default;
  OutputFile(const OutputFile &) = delete;
  OutputFile &operator=(const OutputFile &) = delete;
  llvm::SmallString<256> temporary;
  bool create(const std::string &destination) {
    if (auto ec = llvm::sys::fs::createUniqueFile(destination + ".tmp-%%%%%%",
                                                  temporary)) {
      error("cannot create output '" + destination + "': " + ec.message());
      return false;
    }
    return true;
  }
  bool publish(const std::string &destination) {
    if (auto ec = llvm::sys::fs::rename(temporary, destination)) {
      error("cannot publish output '" + destination + "': " + ec.message());
      return false;
    }
    temporary.clear();
    return true;
  }
  ~OutputFile() {
    if (!temporary.empty())
      llvm::sys::fs::remove(temporary);
  }
};

int execute(const std::string &tool, const std::vector<std::string> &arguments,
            bool verbose, const std::string &stdoutFile = {}) {
  std::vector<llvm::StringRef> args{tool};
  for (const auto &argument : arguments)
    args.push_back(argument);
  if (verbose) {
    std::cerr << std::quoted(tool);
    for (const auto &argument : arguments)
      std::cerr << ' ' << std::quoted(argument);
    if (!stdoutFile.empty())
      std::cerr << " > " << std::quoted(stdoutFile);
    std::cerr << '\n';
  }
  std::vector<std::optional<llvm::StringRef>> redirects;
  if (!stdoutFile.empty())
    redirects = {std::nullopt, stdoutFile, std::nullopt};
  std::string message;
  int status = llvm::sys::ExecuteAndWait(tool, args, std::nullopt, redirects, 0,
                                         0, &message);
  if (status != 0) {
    std::cerr << "cc1: error: '" << tool << "' failed";
    if (status >= 0)
      std::cerr << " with exit status " << status;
    else
      std::cerr << " (execution failure or signal)";
    if (!message.empty())
      std::cerr << ": " << message;
    std::cerr << '\n';
    return status > 0 && status <= 255 ? status : 1;
  }
  return 0;
}

bool checkOutput(const std::string &output, const DriverOptions &options) {
  if (llvm::sys::fs::is_directory(output)) {
    error("output is a directory: '" + output + "'");
    return false;
  }
  for (const auto &input : options.inputs) {
    if (input.kind != InputKind::LinkOption &&
        llvm::sys::fs::equivalent(input.path, output)) {
      error("output would overwrite input '" + input.path + "'");
      return false;
    }
  }
  return true;
}

int compile(const Input &input, size_t index, const std::string &output,
            const DriverOptions &options, const Toolchain &tools,
            const Workspace &workspace, bool assembly) {
  auto raw = workspace.file(index, ".mlir");
  auto lowered = workspace.file(index, ".llvm.mlir");
  auto ir = workspace.file(index, ".ll");
  std::vector<std::string> frontend{"-cc1", "--emit-mlir"};
  if (!options.diagnosticColor.empty())
    frontend.push_back(options.diagnosticColor);
  for (const auto &directory : options.includeDirs)
    frontend.push_back("-I" + directory);
  frontend.push_back("--");
  frontend.push_back(input.path);
  if (int status = execute(tools.cc1, frontend, options.verbose, raw))
    return status;
  if (int status = execute(tools.optimizer,
                           {raw, "--convert-tensorium-to-llvm", "-o", lowered},
                           options.verbose))
    return status;
  if (int status =
          execute(tools.translator, {"--mlir-to-llvmir", lowered, "-o", ir},
                  options.verbose))
    return status;
  return execute(tools.clang,
                 {options.optimization, assembly ? "-S" : "-c", "-x", "ir", ir,
                  "-o", output},
                 options.verbose);
}
} // namespace

int runDriver(int argc, char **argv) {
  DriverOptions options;
  if (!parseOptions(argc, argv, options, std::cerr))
    return 1;
  if (options.help) {
    printHelp(std::cout, llvm::sys::path::filename(argv[0]).str());
    return 0;
  }
  if (options.version) {
    std::cout << llvm::sys::path::filename(argv[0]).str() << ' '
              << TENSORIUM_DRIVER_VERSION << '\n';
    return 0;
  }
  for (auto &input : options.inputs) {
    if (input.kind == InputKind::LinkOption)
      continue;
    input.path = absolute(input.path);
    if (input.path.empty())
      return 1;
    if (!llvm::sys::fs::is_regular_file(input.path))
      return error("input is not a readable regular file: '" + input.path +
                   "'");
  }
  if (options.output == "-")
    return error("-o - is not supported for native output");
  const bool link = options.action == Action::Link;
  std::vector<std::string> outputs(options.inputs.size());
  std::string linkedOutput;
  if (link) {
    linkedOutput = absolute(options.output.empty() ? "a.out" : options.output);
    if (linkedOutput.empty() || !checkOutput(linkedOutput, options))
      return 1;
  } else {
    std::set<std::string> destinations;
    for (size_t i = 0; i < options.inputs.size(); ++i) {
      llvm::SmallString<256> name(
          llvm::sys::path::filename(options.inputs[i].path));
      llvm::sys::path::replace_extension(
          name, options.action == Action::Assembly ? "s" : "o");
      outputs[i] =
          absolute(options.output.empty() ? std::string(name) : options.output);
      if (outputs[i].empty() || !checkOutput(outputs[i], options))
        return 1;
      if (!destinations.insert(outputs[i]).second)
        return error("multiple inputs produce the same output '" + outputs[i] +
                     "'");
    }
  }
  auto executable =
      llvm::sys::fs::getMainExecutable(argv[0], &executableAnchor);
  auto tools = discoverToolchain(options, executable, std::cerr);
  if (!tools)
    return 1;
  Workspace workspace;
  if (!workspace.create())
    return 1;
  std::vector<std::string> linkArguments;
  for (size_t i = 0; i < options.inputs.size(); ++i) {
    const auto &input = options.inputs[i];
    if (input.kind != InputKind::Source) {
      linkArguments.push_back(input.path);
      continue;
    }
    if (link) {
      auto object = workspace.file(i, ".o");
      if (int status =
              compile(input, i, object, options, *tools, workspace, false))
        return status;
      linkArguments.push_back(object);
    } else {
      OutputFile output;
      if (!output.create(outputs[i]))
        return 1;
      if (int status =
              compile(input, i, std::string(output.temporary), options, *tools,
                      workspace, options.action == Action::Assembly))
        return status;
      if (!output.publish(outputs[i]))
        return 1;
    }
  }
  if (!link)
    return 0;
  OutputFile output;
  if (!output.create(linkedOutput))
    return 1;
  linkArguments.push_back("-o");
  linkArguments.emplace_back(output.temporary);
  if (int status = execute(tools->clang, linkArguments, options.verbose))
    return status;
  return output.publish(linkedOutput) ? 0 : 1;
}
} // namespace cc1::driver
