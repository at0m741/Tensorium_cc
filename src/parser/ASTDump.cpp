#include "ASTDump.hpp"
#include "AST.hpp"
#include "Type.hpp"
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

const char *unaryName(UnaryOp op) {
  static constexpr const char *names[] = {
      "-", "!", "~", "*", "&", "pre++", "pre--", "post++", "post--", "+",
  };
  constexpr unsigned count = sizeof(names) / sizeof(names[0]);
  static_assert(count == static_cast<unsigned>(UnaryOp::POS) + 1);
  const unsigned index = static_cast<unsigned>(op);
  return index < count ? names[index] : "?";
}

std::string escapedLiteral(const std::string &value, char quote) {
  std::string result(1, quote);
  for (unsigned char ch : value) {
    switch (ch) {
    case '\0':
      result += "\\0";
      break;
    case '\n':
      result += "\\n";
      break;
    case '\r':
      result += "\\r";
      break;
    case '\t':
      result += "\\t";
      break;
    case '\\':
      result += "\\\\";
      break;
    default:
      if (ch == static_cast<unsigned char>(quote)) {
        result += '\\';
        result += static_cast<char>(ch);
      } else if (ch < 32 || ch >= 127) {
        static constexpr char hex[] = "0123456789abcdef";
        result += "\\x";
        result += hex[ch >> 4];
        result += hex[ch & 15];
      } else {
        result += static_cast<char>(ch);
      }
    }
  }
  result += quote;
  return result;
}
} // namespace

void dumpAST(const Node *node, std::ostream &out, unsigned depth, bool typed) {
  if (!node)
    return;

  out << std::string(depth * 2, ' ');

  if (typed) {
    if (auto *expr = dynamic_cast<const Expr *>(node))
      out << "[" << (expr->type ? expr->type->str() : "<untyped>")
          << (expr->isLval ? ", lvalue] " : ", rvalue] ");
    else if (auto *decl = dynamic_cast<const Decl *>(node))
      out << "[" << (decl->type ? decl->type->str() : "<untyped>") << "] ";
  }

  auto child = [&](const Node *n) { dumpAST(n, out, depth + 1, typed); };

  if (auto *tu = dynamic_cast<const TranslationUnit *>(node)) {
    out << "TranslationUnit\n";
    for (auto *decl : tu->decls)
      child(decl);

  } else if (auto *fn = dynamic_cast<const FuncDecl *>(node)) {
    out << "FuncDecl " << fn->name << '\n';
    for (auto *param : fn->params)
      child(param);
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
    out << "BinaryExpr " << binaryName(bin->op);
    if (typed && bin->computationType)
      out << " compute=" << bin->computationType->str();
    out << '\n';
    child(bin->lhs);
    child(bin->rhs);

  } else if (auto *unary = dynamic_cast<const UnaryExpr *>(node)) {
    out << "UnaryExpr " << unaryName(unary->op) << '\n';
    child(unary->operand);

  } else if (auto *ternary = dynamic_cast<const TernaryExpr *>(node)) {
    out << "TernaryExpr\n";
    child(ternary->cond);
    child(ternary->then);
    child(ternary->els);

  } else if (auto *call = dynamic_cast<const CallExpr *>(node)) {
    out << "CallExpr\n";
    child(call->callee);
    for (auto *arg : call->args)
      child(arg);

  } else if (auto *index = dynamic_cast<const IndexExpr *>(node)) {
    out << "IndexExpr\n";
    child(index->base);
    child(index->index);

  } else if (auto *member = dynamic_cast<const MemberExpr *>(node)) {
    out << "MemberExpr " << (member->arrow ? "->" : ".") << member->member
        << '\n';
    child(member->base);

  } else if (auto *size = dynamic_cast<const SizeofExpr *>(node)) {
    if (size->ofType)
      out << "SizeofExpr type " << size->operandType->str() << '\n';
    else {
      out << "SizeofExpr expr\n";
      child(size->expr);
    }

  } else if (auto *cast = dynamic_cast<const ImplicitCastExpr *>(node)) {
    static constexpr const char *names[] = {"lvalue-to-rvalue",
                                            "array-to-pointer",
                                            "function-to-pointer",
                                            "arithmetic",
                                            "pointer",
                                            "null-to-pointer",
                                            "default-argument-promotion"};
    out << "ImplicitCastExpr " << names[static_cast<unsigned>(cast->kind)]
        << '\n';
    child(cast->operand);

  } else if (auto *cast = dynamic_cast<const CastExpr *>(node)) {
    out << "CastExpr " << cast->toType->str() << '\n';
    child(cast->operand);

  } else if (auto *lit = dynamic_cast<const IntLitExpr *>(node)) {
    out << "IntLitExpr " << lit->val << '\n';

  } else if (auto *lit = dynamic_cast<const FloatLitExpr *>(node)) {
    out << "FloatLitExpr " << lit->val << '\n';

  } else if (auto *lit = dynamic_cast<const CharLitExpr *>(node)) {
    out << "CharLitExpr " << escapedLiteral(std::string(1, lit->val), '\'')
        << '\n';

  } else if (auto *lit = dynamic_cast<const StringLitExpr *>(node)) {
    out << "StringLitExpr " << escapedLiteral(lit->val, '"') << '\n';

  } else if (auto *ident = dynamic_cast<const IdentExpr *>(node)) {
    out << "IdentExpr " << ident->name << '\n';

  } else if (auto *var = dynamic_cast<const VarDecl *>(node)) {
    out << "VarDecl " << var->name << '\n';
    child(var->init);

  } else if (auto *td = dynamic_cast<const TypedefDecl *>(node)) {
    out << "TypedefDecl " << td->name << '\n';

  } else if (auto *param = dynamic_cast<const ParamDecl *>(node)) {
    out << "ParamDecl " << (param->name.empty() ? "<unnamed>" : param->name)
        << '\n';

  } else if (auto *tag = dynamic_cast<const StructDecl *>(node)) {
    out << "StructDecl " << tag->name << '\n';

  } else if (auto *tag = dynamic_cast<const UnionDecl *>(node)) {
    out << "UnionDecl " << tag->name << '\n';

  } else if (auto *tag = dynamic_cast<const EnumDecl *>(node)) {
    out << "EnumDecl " << tag->name << '\n';

  } else {
    out << "UnsupportedNode\n";
  }
}
