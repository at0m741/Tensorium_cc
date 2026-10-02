#include "ASTDump.hpp"
#include "AST.hpp"
#include <ostream>
#include <string>

namespace {
const char *binaryName(BinaryOp op) {
  static constexpr const char *names[] = {
      "+",  "-",  "*",  "/",  "%",  "&&", "||", "&",   "|",   "^",
      "<<", ">>", "==", "!=", "<",  ">",  "<=", ">=",  "=",   "+=",
      "-=", "*=", "/=", "%=", "&=", "|=", "^=", "<<=", ">>=", ",",
  };

  constexpr unsigned count = sizeof(names) / sizeof(names[0]);
  static_assert(count == static_cast<unsigned>(BinaryOp::COMMA) + 1);
  const unsigned index = static_cast<unsigned>(op);

  return index < count ? names[index] : "?";
}
} // namespace

void dumpAST(const Node *node, std::ostream &out, unsigned depth) {
  if (!node)
    return;

  out << std::string(depth * 2, ' ');

  auto child = [&](const Node *n) { dumpAST(n, out, depth + 1); };

  if (auto *tu = dynamic_cast<const TranslationUnit *>(node)) {
    out << "TranslationUnit\n";
    for (auto *decl : tu->decls)
      child(decl);

  } else if (auto *fn = dynamic_cast<const FuncDecl *>(node)) {
    out << "FuncDecl " << fn->name << '\n';
    child(fn->body);

  } else if (auto *block = dynamic_cast<const CompoundStmt *>(node)) {
    out << "CompoundStmt\n";
    for (auto *item : block->items)
      child(item);

  } else if (auto *ret = dynamic_cast<const ReturnStmt *>(node)) {
    out << "ReturnStmt\n";
    child(ret->value);

  } else if (auto *stmt = dynamic_cast<const ExprStmt *>(node)) {
    out << "ExprStmt\n";
    child(stmt->expr);

  } else if (auto *bin = dynamic_cast<const BinaryExpr *>(node)) {
    out << "BinaryExpr " << binaryName(bin->op) << '\n';
    child(bin->lhs);
    child(bin->rhs);

  } else if (auto *lit = dynamic_cast<const IntLitExpr *>(node)) {
    out << "IntLitExpr " << lit->val << '\n';

  } else if (auto *lit = dynamic_cast<const FloatLitExpr *>(node)) {
    out << "FloatLitExpr " << lit->val << '\n';

  } else if (auto *ident = dynamic_cast<const IdentExpr *>(node)) {
    out << "IdentExpr " << ident->name << '\n';

  } else if (auto *var = dynamic_cast<const VarDecl *>(node)) {
    out << "VarDecl " << var->name << '\n';
    child(var->init);

  } else if (auto *td = dynamic_cast<const TypedefDecl *>(node)) {
    out << "TypedefDecl " << td->name << '\n';

  } else {
    out << "UnsupportedNode\n";
  }
}
