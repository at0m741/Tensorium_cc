#include "cc1/AST/ASTContext.hpp"
#include "cc1/AST/ASTDump.hpp"
#include "cc1/Lex/Lexer.hpp"
#include "cc1/Parse/Parser.hpp"
#include "cc1/Sema/Sema.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace {
void expect(bool condition, const char *message) {
  if (!condition)
    throw std::runtime_error(message);
}

struct TrackedExpr : Expr {
  explicit TrackedExpr(int &destroyed) : destroyed(destroyed) {}
  ~TrackedExpr() override { ++destroyed; }
  int &destroyed;
};

void destroysSharedNodesOnce() {
  int destroyed = 0;
  {
    ASTContext ast;
    auto *shared = ast.create<TrackedExpr>(destroyed);
    shared->type = ast.getTypes().intTy();
    auto *expression = ast.create<BinaryExpr>();
    expression->lhs = shared;
    expression->rhs = shared;
    // Growing the context must preserve pointers in the AST graph.
    for (int i = 0; i < 1024; ++i)
      ast.create<TrackedExpr>(destroyed);
    expect(expression->lhs == shared && expression->rhs == shared,
           "context growth invalidated an AST pointer");
    expect(destroyed == 0, "node was destroyed before its context");
  }
  expect(destroyed == 1025, "context did not destroy each node exactly once");
}

void conversionsOutliveParserAndSema() {
  const std::string source = "int promote(short x) { return x + 1; }";
  std::ostringstream diagnostics;
  DiagnosticEngine diag("lifetime.c", source, diagnostics,
                        DiagnosticColor::Never);
  ASTContext ast;
  TargetInfo target = TargetInfo::x86_64();
  const LangOptions options = LangOptions::forc99();
  TranslationUnit *unit = nullptr;
  {
    Lexer lexer(source, "lifetime.c", diag, options);
    Parser parser(lexer, ast, diag, target, options);
    unit = parser.parse();
    Sema sema(ast, diag, target);
    expect(!diag.hasErrors() && sema.analyze(*unit),
           "failed to analyze lifetime fixture");
  }
  auto *function = dynamic_cast<FuncDecl *>(unit->decls.front());
  auto *returnStmt = dynamic_cast<ReturnStmt *>(function->body->items.front());
  auto *sum = dynamic_cast<BinaryExpr *>(returnStmt->value);
  auto *promotion = dynamic_cast<ImplicitCastExpr *>(sum->lhs);
  expect(promotion && promotion->kind == ImplicitCastKind::Arithmetic,
         "lost integer promotion after destroying Sema");
  auto *load = dynamic_cast<ImplicitCastExpr *>(promotion->operand);
  expect(load && load->kind == ImplicitCastKind::LValueToRValue,
         "lost parameter conversion after destroying Sema");
  auto *reference = dynamic_cast<IdentExpr *>(load->operand);
  expect(reference && reference->decl == function->params.front(),
         "lost declaration identity after destroying Parser");
  std::ostringstream dump;
  dumpAST(unit, dump, 0, true);
  expect(dump.str().find("ImplicitCastExpr") != std::string::npos,
         "cannot traverse typed AST after destroying Sema");
}
} // namespace

int main() {
  try {
    destroysSharedNodesOnce();
    conversionsOutliveParserAndSema();
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
  return 0;
}
