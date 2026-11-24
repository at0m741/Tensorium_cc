#include "cc1/AST/ASTContext.hpp"
#include "cc1/CodeGen/LocalStorageAnalysis.hpp"
#include "cc1/Frontend/CompilerInstance.hpp"
#include "cc1/Frontend/CompilerInvocation.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace {
void expect(bool condition, const std::string &message) {
  if (!condition)
    throw std::runtime_error(message);
}

CompilerInvocation invocation() {
  CompilerInvocation result;
  result.inputFile =
      std::string(CC1_TEST_SOURCE_DIR) + "/CodeGen/LocalStorage/parameters.c";
  result.action = FrontendAction::CheckSyntax;
  result.diagnosticColor = DiagnosticColor::Never;
  result.target = TargetInfo::x86_64();
  return result;
}

const FuncDecl &function(const TranslationUnit &unit, const std::string &name) {
  for (const auto *decl : unit.decls)
    if (auto *fn = dynamic_cast<const FuncDecl *>(decl))
      if (fn->name == name)
        return *fn;
  throw std::runtime_error("missing function: " + name);
}

void checksTypedCFixture() {
  std::ostringstream output, diagnostics;
  CompilerInstance compiler(invocation(), output, diagnostics);
  expect(compiler.execute(), diagnostics.str());
  const auto *unit = compiler.getTranslationUnit();
  expect(unit != nullptr, "frontend did not retain the typed AST");

  struct Case {
    const char *name;
    std::vector<bool> storage;
  };
  const Case cases[] = {
      {"choose", {false, false}},
      {"reassigned", {false, true}},
      {"compound_assignments",
       {true, true, true, true, true, true, true, true, true, true, false}},
      {"updates", {true, true, true, true}},
      {"address_scalar", {true}},
      {"pointer_write", {false, false}},
      {"pointer_rebind", {true, false}},
      {"pointer_update", {true, false}},
      {"pointer_address", {true}},
      {"pointer_element", {false, false}},
      {"pointee_address", {false, false}},
      {"index_update", {false, true}},
      {"sizeof_only", {false, false}},
      {"initializer_write", {true}},
      {"initializer_address", {true}},
      {"control_flow", {false, false}},
      {"for_initializer_write", {true}},
      {"for_condition_write", {true}},
      {"for_increment_write", {true}},
      {"while_write", {true}},
      {"call_update", {true}},
      {"call_pointer", {false, true}},
      {"conditional_write", {false, true, true}},
      {"logical_write", {false, true}},
      {"comma_write", {true, false}},
      {"shadowed", {false}},
      {"volatile_scalar", {true}},
      {"volatile_pointer", {true}},
      {"volatile_pointee", {false}},
      {"adjusted_array", {false}},
      {"const_scalar", {false}},
      {"arithmetic", {false, false}},
      {"consume", {true}},        // A prototype has no body to analyze.
      {"choose", {false, false}}, // Conservative state must reset per function.
  };

  LocalStorageAnalysis analysis;
  expect(analysis.needsStorage(nullptr), "null declaration was treated as SSA");
  for (const auto &test : cases) {
    const auto &fn = function(*unit, test.name);
    analysis.analyze(fn);
    expect(fn.params.size() == test.storage.size(),
           std::string(test.name) + ": unexpected parameter count");
    for (size_t i = 0; i < fn.params.size(); ++i)
      expect(analysis.needsStorage(fn.params[i]) == test.storage[i],
             std::string(test.name) + ": wrong storage decision for " +
                 fn.params[i]->name);
  }

  const auto &fn = function(*unit, "initializer_write");
  analysis.analyze(fn);
  const auto *local = dynamic_cast<const VarDecl *>(fn.body->items.front());
  expect(local && !analysis.needsStorage(local),
         "initialized read-only local requires storage");
  const auto &other = function(*unit, "choose");
  expect(analysis.needsStorage(other.params.front()),
         "declaration from another function was treated as SSA");
}

void keepsUnknownConstructsInMemory() {
  // Synthetic nodes exercise the fallback for future AST extensions.
  struct UnknownStmt : Stmt {};
  struct UnknownExpr : Expr {};

  ASTContext ast;
  ParamDecl parameter;
  parameter.type = ast.getTypes().intTy();
  CompoundStmt body;
  FuncDecl fn;
  fn.params.push_back(&parameter);
  fn.body = &body;

  LocalStorageAnalysis analysis;
  analysis.analyze(fn);
  expect(!analysis.needsStorage(&parameter), "unused scalar requires storage");

  UnknownStmt unknownStmt;
  body.items.push_back(&unknownStmt);
  analysis.analyze(fn);
  expect(analysis.needsStorage(&parameter), "unknown statement was ignored");

  UnknownExpr unknownExpr;
  ReturnStmt ret;
  ret.value = &unknownExpr;
  body.items = {&ret};
  analysis.analyze(fn);
  expect(analysis.needsStorage(&parameter), "unknown expression was ignored");

  body.items.clear();
  analysis.analyze(fn);
  expect(!analysis.needsStorage(&parameter), "fallback leaked across analyses");

  parameter.type = ast.getTypes().arrayOf(ast.getTypes().intTy(), 4);
  analysis.analyze(fn);
  expect(analysis.needsStorage(&parameter), "non-scalar parameter used SSA");
  parameter.type = nullptr;
  expect(analysis.needsStorage(&parameter), "untyped declaration used SSA");
}
} // namespace

int main() {
  try {
    checksTypedCFixture();
    keepsUnknownConstructsInMemory();
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
  return 0;
}
