#pragma once

#include "cc1/AST/ASTContext.hpp"
#include "cc1/Basic/SourceManager.hpp"
#include "cc1/Frontend/CompilerInvocation.hpp"
#include <iosfwd>

// Owns the source buffers and AST for one compiler invocation.
class CompilerInstance {
public:
  CompilerInstance(CompilerInvocation invocation, std::ostream &output,
                   std::ostream &diagnosticOutput);

  static bool hasMLIRSupport();
  bool execute();
  const TranslationUnit *getTranslationUnit() const { return unit; }

private:
  CompilerInvocation invocation;
  std::ostream &output;
  std::ostream &diagnosticOutput;
  DiagnosticEngine diagnostics;
  SourceManager sources;
  ASTContext ast;
  TranslationUnit *unit = nullptr;
};
