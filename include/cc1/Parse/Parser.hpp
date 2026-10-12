#pragma once
#include "cc1/AST/ASTContext.hpp"
#include "cc1/Basic/Diagnostic.hpp"
#include "cc1/Basic/LangOptions.hpp"
#include "cc1/Basic/TargetInfo.hpp"
#include "cc1/Lex/TokenSource.hpp"
#include <string>
#include <unordered_map>
#include <vector>

class ParserTestHelper;

class Parser {
public:
  Parser(TokenSource &lexer, ASTContext &ast, DiagnosticEngine &diag,
         const TargetInfo &target, const LangOptions &opts);

  // The returned tree is owned by the ASTContext passed to the constructor.
  TranslationUnit *parse();

private:
  friend class ParserTestHelper;

  TokenSource &lex;
  ASTContext &ast;
  TypePool &types;
  DiagnosticEngine &_diag;
  const TargetInfo &target;
  const LangOptions &opts;

  Token _cur;
  Token _peek;

  std::unordered_map<std::string, Type *> _typedefs;
  // Each nested function declarator has its own parameter declarations.
  std::unordered_map<Type *, std::vector<ParamDecl *>> _functionParams;

  Token advance();
  Token expect(TokenKind k, const char *msg);
  bool check(TokenKind k) const;
  bool match(TokenKind k);
  bool isTypeName() const;
  bool isTypeName(const Token &token) const;

  void error(const SourceLoc &loc, const std::string &msg);
  void syncToNextDecl();

  Decl *parseDecl(bool global = true);
  FuncDecl *parseFuncDecl(Type *retType, const std::string &name,
                          SourceLoc &loc);
  VarDecl *parseVarDecl(Type *baseType, const std::string &name, SourceLoc &loc,
                        bool global);

  Type *parseSuffix(Type *base);
  void replacePlaceholder(Type *node, Type *placeholder, Type *replacement);
  Type *parseTypeSpec();
  Type *parseDeclarator(Type *base, std::string &nameOut);
  Type *parsePointer(Type *base);
  Type *parseFuncType(Type *retType);
  Type *parseArrayType(Type *elemType);
  StorageClass parseStorageClass();
  Qualifiers parseQualifiers();
  Type *parseStructOrUnion();
  Type *parseEnum();

  Stmt *parseStmt();
  CompoundStmt *parseCompoundStmt();
  IfStmt *parseIfStmt();
  WhileStmt *parseWhileStmt();
  ForStmt *parseForStmt();
  ReturnStmt *parseReturnStmt();

  Expr *parseExpr(int minPrec = 0);
  Expr *parseUnary();
  Expr *parsePrimary();
  Expr *parsePostfix(Expr *base);
  Expr *parseCast();

  int infixPrec(TokenKind k) const;
  bool isRightAssoc(TokenKind k) const;
  BinaryOp tokenToBinaryOp(TokenKind k) const;
};
