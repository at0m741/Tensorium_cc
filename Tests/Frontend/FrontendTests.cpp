#include "cc1/AST/ASTDump.hpp"
#include "cc1/Frontend/CompilerInstance.hpp"
#include "cc1/Frontend/CompilerInvocation.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace {
void expect(bool condition, const char *message) {
  if (!condition)
    throw std::runtime_error(message);
}

CompilerInvocation invocation(FrontendAction action) {
  CompilerInvocation result;
  result.inputFile = std::string(CC1_TEST_SOURCE_DIR) + "/Frontend/input.c";
  result.action = action;
  result.diagnosticColor = DiagnosticColor::Never;
  result.target = TargetInfo::x86_64();
  return result;
}

void capturesOutputAndRetainsTheTypedAST() {
  std::ostringstream output, diagnostics;
  CompilerInstance compiler(invocation(FrontendAction::CheckSyntax), output,
                            diagnostics);
  expect(compiler.execute(), "frontend rejected valid input");
  expect(output.str().empty() && diagnostics.str().empty(),
         "syntax checking produced unexpected output");
  const auto *unit = compiler.getTranslationUnit();
  expect(unit && unit->decls.size() == 1, "frontend did not retain its AST");
  // Parser and Sema have been destroyed when execute() returns.
  std::ostringstream dump;
  dumpAST(unit, dump, 0, true);
  expect(dump.str().find("ImplicitCastExpr") != std::string::npos,
         "typed AST cannot be read after compilation");
}

void emitsIntoTheCallersStream() {
  for (auto action : {FrontendAction::Preprocess, FrontendAction::DumpTokens,
                      FrontendAction::DumpAST, FrontendAction::DumpSema}) {
    std::ostringstream output, diagnostics;
    CompilerInstance compiler(invocation(action), output, diagnostics);
    expect(compiler.execute(), "frontend action failed");
    expect(!output.str().empty() && diagnostics.str().empty(),
           "frontend action did not use the requested output stream");
  }
  std::ostringstream output, diagnostics;
  CompilerInstance compiler(invocation(FrontendAction::EmitMLIR), output,
                            diagnostics);
  if (CompilerInstance::hasMLIRSupport()) {
    expect(compiler.execute(), "MLIR emission failed");
    expect(output.str().find("func.func @promote") != std::string::npos,
           "MLIR emission did not use the requested output stream");
    expect(diagnostics.str().empty(), "MLIR emission produced diagnostics");
  } else {
    expect(!compiler.execute(), "disabled MLIR emission succeeded");
    expect(output.str().empty() &&
               diagnostics.str().find("MLIR support is disabled") !=
                   std::string::npos,
           "disabled MLIR emission did not report its diagnostic");
  }
}

void capturesFileErrors() {
  auto options = invocation(FrontendAction::CheckSyntax);
  options.inputFile += ".missing";
  std::ostringstream output, diagnostics;
  CompilerInstance compiler(options, output, diagnostics);
  expect(!compiler.execute(), "missing file did not fail");
  expect(output.str().empty() && compiler.getTranslationUnit() == nullptr,
         "file error left a partial AST or output");
  expect(diagnostics.str().find("cc1: error: cannot open") != std::string::npos,
         "file error escaped the requested diagnostic stream");
}
} // namespace

int main() {
  try {
    capturesOutputAndRetainsTheTypedAST();
    emitsIntoTheCallersStream();
    capturesFileErrors();
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
  return 0;
}
