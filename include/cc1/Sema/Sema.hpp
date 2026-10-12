#pragma once
#include "cc1/AST/ASTContext.hpp"
#include "cc1/Basic/Diagnostic.hpp"
#include "cc1/Basic/TargetInfo.hpp"
#include "cc1/Sema/SymbolTable.hpp"
#include <unordered_map>

// A successful run annotates every expression and resolves every identifier.
// Inserted conversions belong to ASTContext and outlive this analyzer.
class Sema {
public:
  Sema(ASTContext &ast, DiagnosticEngine &diagnostics, const TargetInfo &target)
      : ast(ast), types(ast.getTypes()), diag(diagnostics), target(target) {}
  bool analyze(TranslationUnit &unit);

private:
  ASTContext &ast;
  TypePool &types;
  DiagnosticEngine &diag;
  const TargetInfo &target;
  SymbolTable symbols;
  std::unordered_map<std::string, Type *> tags;
  std::unordered_map<Decl *, FuncDecl *> definitions;
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
