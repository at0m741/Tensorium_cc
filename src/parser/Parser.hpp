#pragma once
#include "../../include/cc1/Diagnostic.hpp"
#include "../../include/cc1/LangOptions.hpp"
#include "../../include/cc1/TargetInfo.hpp"
#include "../lexer/Lexer.hpp"
#include "AST.hpp"
#include "Type.hpp"
#include <string>
#include <unordered_map>
#include <vector>

class ParserTestHelper;

class Parser {
public:
  Parser(Lexer &lexer, TypePool &types, Diagnostic &diag,
         const TargetInfo &target, const LangOptions &opts);

  TranslationUnit *parse();

private:
  friend class ParserTestHelper;

  Lexer &lex;
  TypePool &types;
  Diagnostic &_diag;
  const TargetInfo &target;
  const LangOptions &opts;

  Token _cur;
  Token _peek;

  std::unordered_map<std::string, Type *> _typedefs;

  Token advance();
  Token expect(TokenKind k, const char *msg);
  bool check(TokenKind k) const;
  bool match(TokenKind k);
  bool isTypeName() const;

  void error(const SourceLoc &loc, const std::string &msg);
  void syncToNextDecl();

  Decl *parseDecl();
  FuncDecl *parseFuncDecl(Type *retType, const std::string &name,
                          SourceLoc &loc);
  VarDecl *parseVarDecl(Type *baseType, const std::string &name,
                        SourceLoc &loc);

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
  DoWhileStmt *parseDoWhileStmt();
  ForStmt *parseForStmt();
  ReturnStmt *parseReturnStmt();
  SwitchStmt *parseSwitchStmt();
  GotoStmt *parseGotoStmt();
  LabelStmt *parseLabelStmt(const std::string &label);

  Expr *parseExpr(int minPrec = 0);
  Expr *parseUnary();
  Expr *parsePrimary();
  Expr *parsePostfix(Expr *base);
  Expr *parseCast();

  int infixPrec(TokenKind k) const;
  bool isRightAssoc(TokenKind k) const;
  BinaryOp tokenToBinaryOp(TokenKind k) const;
};

class ParserTestHelper {
public:
  explicit ParserTestHelper(Parser &p) : parser(p) {}

  Type *parseTypeSpec() { return parser.parseTypeSpec(); }
  Type *parseDeclarator(Type *base, std::string &nameOut) {
    return parser.parseDeclarator(base, nameOut);
  }
  Type *parsePointer(Type *base) { return parser.parsePointer(base); }
  Expr *parseExpr() { return parser.parseExpr(); }

  TokenKind currentKind() const { return parser._cur.kind; }

private:
  Parser &parser;
};
