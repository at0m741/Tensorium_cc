#pragma once

#include "cc1/Basic/Diagnostic.hpp"
#include "cc1/Basic/LangOptions.hpp"
#include "cc1/Basic/TargetInfo.hpp"
#include <iosfwd>
#include <optional>
#include <string>
#include <vector>

enum class FrontendAction {
  CheckSyntax,
  Preprocess,
  DumpTokens,
  DumpAST,
  DumpSema,
  EmitMLIR
};

struct CompilerInvocation {
  std::string inputFile;
  FrontendAction action = FrontendAction::CheckSyntax;
  DiagnosticColor diagnosticColor = DiagnosticColor::Auto;
  LangOptions langOptions = LangOptions::forc99();
  std::vector<std::string> includeDirs;
  std::optional<TargetInfo> target;
};

bool parseCompilerInvocation(int argc, char **argv,
                             CompilerInvocation &invocation,
                             std::ostream &diagnosticOutput);
