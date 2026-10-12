#include "cc1/Frontend/CompilerInvocation.hpp"
#include "cc1/Frontend/CompilerInstance.hpp"
#include <ostream>
#include <utility>

bool parseCompilerInvocation(int argc, char **argv,
                             CompilerInvocation &invocation,
                             std::ostream &diagnosticOutput) {
  DiagnosticColor color = DiagnosticColor::Auto;
  bool dumpTokens = false;
  bool dumpAst = false;
  bool dumpSema = false;
  bool preprocessOnly = false;
  bool emitMlir = false;
  std::vector<std::string> includeDirs;
  const char *filename = nullptr;
  bool options = true;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (options && arg == "--") {
      options = false;
    } else if (options && arg == "-fcolor-diagnostics") {
      color = DiagnosticColor::Always;
    } else if (options && arg == "-fno-color-diagnostics") {
      color = DiagnosticColor::Never;
    } else if (options && arg == "--dump-tokens") {
      dumpTokens = true;
    } else if (options && arg == "--dump-ast") {
      dumpAst = true;
    } else if (options && arg == "--dump-sema") {
      dumpSema = true;
    } else if (options && arg == "--emit-mlir") {
      if (!CompilerInstance::hasMLIRSupport()) {
        DiagnosticEngine diagnostics("cc1", "", diagnosticOutput, color);
        diagnostics.error({}, "MLIR support is disabled; rebuild with "
                              "-DCC1_ENABLE_MLIR=ON");
        return false;
      }
      emitMlir = true;
    } else if (options && arg == "-E") {
      preprocessOnly = true;
    } else if (options && (arg == "-I" || arg.rfind("-I", 0) == 0)) {
      if (arg == "-I") {
        if (i + 1 == argc) {
          DiagnosticEngine diagnostics("cc1", "", diagnosticOutput, color);
          diagnostics.error({}, "expected directory after -I");
          return false;
        }
        includeDirs.emplace_back(argv[++i]);
      } else {
        includeDirs.push_back(arg.substr(2));
      }
    } else if ((options && !arg.empty() && arg[0] == '-') || filename) {
      DiagnosticEngine diagnostics("cc1", "", diagnosticOutput, color);
      diagnostics.error({}, filename ? "expected a single input file"
                                     : "unknown option '" + arg + "'");
      return false;
    } else {
      filename = argv[i];
    }
  }
  if (!filename) {
    DiagnosticEngine diagnostics("cc1", "", diagnosticOutput, color);
    diagnostics.error({}, "no input file");
    diagnosticOutput
        << "usage: cc1 "
           "[-E|--dump-tokens|--dump-ast|--dump-sema|--emit-mlir] [-I "
           "directory] "
           "[-fcolor-diagnostics|-fno-color-diagnostics] <file.c>\n";
    return false;
  }

  if (static_cast<int>(dumpTokens) + static_cast<int>(dumpAst) +
          static_cast<int>(dumpSema) + static_cast<int>(emitMlir) >
      1) {
    DiagnosticEngine diagnostics("cc1", "", diagnosticOutput, color);
    diagnostics.error({}, "output options cannot be combined");
    return false;
  }
  if (preprocessOnly && (dumpTokens || dumpAst || dumpSema || emitMlir)) {
    DiagnosticEngine diagnostics("cc1", "", diagnosticOutput, color);
    diagnostics.error({}, "-E cannot be combined with another output option");
    return false;
  }

  invocation.inputFile = filename;
  invocation.diagnosticColor = color;
  invocation.includeDirs = std::move(includeDirs);
  invocation.action = preprocessOnly ? FrontendAction::Preprocess
                      : dumpTokens   ? FrontendAction::DumpTokens
                      : dumpAst      ? FrontendAction::DumpAST
                      : dumpSema     ? FrontendAction::DumpSema
                      : emitMlir     ? FrontendAction::EmitMLIR
                                     : FrontendAction::CheckSyntax;
  return true;
}
