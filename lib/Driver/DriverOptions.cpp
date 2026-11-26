#include "cc1/Driver/DriverOptions.hpp"
#include <ostream>

namespace cc1::driver {
bool isFrontendInvocation(int argc, char **argv) {
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--")
      break;
    if (arg == "-o" || arg == "-I" || arg == "-L" || arg == "-l") {
      ++i; // An option value can happen to look like a frontend flag.
      continue;
    }
    if (arg == "-fsyntax-only" || arg == "-E" || arg == "--emit-mlir" ||
        arg == "--dump-tokens" || arg == "--dump-ast" || arg == "--dump-sema")
      return true;
  }
  return false;
}
namespace {
bool endsWith(const std::string &value, const std::string &suffix) {
  return value.size() >= suffix.size() &&
         value.compare(value.size() - suffix.size(), suffix.size(), suffix) ==
             0;
}
} // namespace

bool parseOptions(int argc, char **argv, DriverOptions &options,
                  std::ostream &errors) {
  auto fail = [&](const std::string &message) {
    errors << "cc1: error: " << message << '\n';
    return false;
  };
  bool parseFlags = true, object = false, assembly = false;
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    auto argument = [&](std::string &destination) {
      if (++i == argc || std::string(argv[i]).empty())
        return fail("expected an argument after '" + arg + "'");
      destination = argv[i];
      return true;
    };
    if (parseFlags && arg == "--") {
      parseFlags = false;
    } else if (parseFlags && (arg == "--help" || arg == "-h")) {
      options.help = true;
    } else if (parseFlags && arg == "--version") {
      options.version = true;
    } else if (parseFlags && arg == "-v") {
      options.verbose = true;
    } else if (parseFlags && (arg == "-fcolor-diagnostics" ||
                              arg == "-fno-color-diagnostics")) {
      options.diagnosticColor = arg;
    } else if (parseFlags && arg == "-c") {
      object = true;
    } else if (parseFlags && arg == "-S") {
      assembly = true;
    } else if (parseFlags && arg == "-o") {
      if (!argument(options.output))
        return false;
    } else if (parseFlags && arg.rfind("-o", 0) == 0 && arg.size() > 2) {
      options.output = arg.substr(2);
    } else if (parseFlags && (arg == "-I" || arg.rfind("-I", 0) == 0)) {
      std::string directory = arg.substr(2);
      if (arg == "-I" && !argument(directory))
        return false;
      options.includeDirs.push_back(directory);
    } else if (parseFlags &&
               (arg == "-l" || arg == "-L" || arg.rfind("-l", 0) == 0 ||
                arg.rfind("-L", 0) == 0)) {
      std::string value = arg.substr(2);
      if (arg.size() == 2 && !argument(value))
        return false;
      options.inputs.push_back(
          {arg.substr(0, 2) + value, InputKind::LinkOption});
    } else if (parseFlags && (arg == "-O0" || arg == "-O1" || arg == "-O2" ||
                              arg == "-O3" || arg == "-Os" || arg == "-Oz")) {
      options.optimization = arg;
    } else if (parseFlags && (arg.rfind("--cc1=", 0) == 0 ||
                              arg.rfind("--tensorium-opt=", 0) == 0 ||
                              arg.rfind("--mlir-translate=", 0) == 0 ||
                              arg.rfind("--clang=", 0) == 0)) {
      auto equal = arg.find('=');
      auto value = arg.substr(equal + 1);
      if (value.empty())
        return fail("empty tool path in '" + arg + "'");
      if (arg.rfind("--cc1=", 0) == 0)
        options.cc1 = value;
      else if (arg.rfind("--tensorium-opt=", 0) == 0)
        options.optimizer = value;
      else if (arg.rfind("--mlir-translate=", 0) == 0)
        options.translator = value;
      else
        options.clang = value;
    } else if (parseFlags && !arg.empty() && arg.front() == '-') {
      return fail("unsupported option '" + arg + "' (see --help)");
    } else {
      if (endsWith(arg, ".c"))
        options.inputs.push_back({arg, InputKind::Source});
      else if (endsWith(arg, ".o"))
        options.inputs.push_back({arg, InputKind::Object});
      else if (endsWith(arg, ".a"))
        options.inputs.push_back({arg, InputKind::Archive});
      else
        return fail("unsupported input '" + arg + "'; expected .c, .o or .a");
    }
  }
  if (options.help || options.version)
    return true;
  if (object && assembly)
    return fail("-c and -S cannot be combined");
  options.action = object     ? Action::Object
                   : assembly ? Action::Assembly
                              : Action::Link;
  unsigned count = 0;
  for (const auto &input : options.inputs) {
    if (input.kind != InputKind::LinkOption)
      ++count;
    if (options.action != Action::Link && input.kind != InputKind::Source)
      return fail("-c and -S accept only C sources; linker inputs require a "
                  "link action");
  }
  if (!count)
    return fail("no input files");
  if (options.action != Action::Link && !options.output.empty() && count != 1)
    return fail("cannot use one -o output with multiple sources and -c or -S");
  return true;
}

void printHelp(std::ostream &output, const std::string &programName) {
  output
      << "Usage: " << programName
      << " [options] file.c [file.c | file.o | library.a "
         "...]\n"
         "  -c                  Compile each C input to an object file\n"
         "  -S                  Compile each C input to native assembly\n"
         "  -o FILE             Select output (default: a.out when linking)\n"
         "  -I DIR              Add a C include directory\n"
         "  -L DIR, -l NAME     Add a native library search path or library\n"
         "  -O0|-O1|-O2|-O3|-Os|-Oz  Select native backend optimization\n"
         "  -v                  Print commands as they execute\n"
         "  --cc1=PATH --tensorium-opt=PATH --mlir-translate=PATH "
         "--clang=PATH\n"
         "                      Override a pipeline tool\n"
         "  --                  Treat remaining arguments as input filenames\n"
         "  --help, --version   Display driver information\n"
         "Frontend actions: cc1 -fsyntax-only, -E, --emit-mlir, --dump-ast,\n"
         "                  --dump-sema or --dump-tokens.\n"
         "The default action compiles and links for the frontend's host "
         "target.\n";
}
} // namespace cc1::driver
