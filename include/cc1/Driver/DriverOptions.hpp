#pragma once

#include <iosfwd>
#include <string>
#include <vector>

namespace cc1::driver {
enum class Action { Link, Object, Assembly };
enum class InputKind { Source, Object, Archive, LinkOption };
struct Input {
  std::string path;
  InputKind kind;
};
struct DriverOptions {
  Action action = Action::Link;
  std::vector<Input> inputs;
  std::vector<std::string> includeDirs;
  std::string output;
  std::string optimization = "-O0";
  std::string cc1, optimizer, translator, clang;
  std::string diagnosticColor;
  bool help = false;
  bool version = false;
  bool verbose = false;
};
bool parseOptions(int argc, char **argv, DriverOptions &options,
                  std::ostream &errors);
bool isFrontendInvocation(int argc, char **argv);
void printHelp(std::ostream &output, const std::string &programName);
} // namespace cc1::driver
