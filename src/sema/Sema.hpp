#pragma once
#include "SymbolTable.hpp"
#include "cc1/Diagnostic.hpp"
#include "cc1/TargetInfo.hpp"
#include "parser/Type.hpp"
#include <memory>
#include <unordered_map>

// A successful run annotates every expression and resolves every identifier.
// Keep Sema and TypePool alive while consuming the AST: Sema owns inserted
// casts.
class Sema {
public:
  Sema(TypePool &types, DiagnosticEngine &diagnostics, const TargetInfo &target)
      : types(types), diag(diagnostics), target(target) {}
  bool analyze(TranslationUnit &unit);

private:
  TypePool &types;
  DiagnosticEngine &diag;
  const TargetInfo &target;
  SymbolTable symbols;
  std::unordered_map<std::string, Type *> tags;
  std::unordered_map<Decl *, FuncDecl *> definitions;
  std::vector<std::unique_ptr<ImplicitCastExpr>> casts;
  FuncDecl *currentFunction = nullptr;
  unsigned loopDepth = 0;

  bool declare(Decl *decl, bool global);
  void analyzeFunction(FuncDecl *fn);
  void analyzeBlock(CompoundStmt *block, bool nested = true);
  void analyzeNode(Node *node);
  Type *analyzeExpr(Expr *&expr);
  Type *analyzeUnary(UnaryExpr *expr);
  Type *analyzeBinary(BinaryExpr *expr);
  Type *binaryType(BinaryOp op, Expr *&lhs, Expr *&rhs, SourceLoc loc);
  Type *analyzeConditional(TernaryExpr *expr);
  Type *normalizeType(Type *type, SourceLoc loc);
  bool validateType(Type *type, SourceLoc loc);
  bool compatible(Type *a, Type *b, bool ignoreQuals = false) const;
  bool pointerConvertible(Type *to, Type *from) const;
  Type *commonPointer(Type *a, Type *b, bool allowVoid);
  bool nullConstant(const Expr *expr) const;
  bool modifiable(const Expr *expr) const;
  bool objectPointer(Type *type) const;
  Type *unqualified(Type *type);
  Type *promoted(Type *type);
  Type *usualArithmetic(Type *a, Type *b);
  Type *sizeType(bool isSigned = false);
  Type *integerLiteralType(const IntLitExpr *expr);
  void cast(Expr *&expr, Type *to, ImplicitCastKind kind);
  Type *value(Expr *&expr);
  bool convert(Expr *&expr, Type *to, const std::string &context);
  void defaultPromote(Expr *&expr);
  Type *fail(SourceLoc loc, const std::string &message);
};
