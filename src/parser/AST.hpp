#pragma once
#include "../lexer/Token.hpp"
#include <memory>
#include <string>
#include <vector>

struct Type;
struct Decl;
struct Expr;
struct stmt;

struct Node {
  SourceLoc loc;
  virtual ~Node() = default;
};

struct Expr : Node {
  Type *type = nullptr;
  bool isLval = false;
};

struct IntLitExpr : Expr {
  long long val;
};
struct FloatLitExpr : Expr {
  double val;
};
struct CharLitExpr : Expr {
  char val;
};
struct StringLitExpr : Expr {
  std::string val;
};

struct IdentExpr : Expr {
  std::string name;
  Decl *decl = nullptr;
};

enum class UnaryOp {
  NEG,
  NOT,
  BITNOT,
  DEREF,
  ADDR,
  PRE_INC,
  PRE_DEC,
  POST_INC,
  POST_DEC,
};

enum class BinaryOp {
  ADD,
  SUB,
  MUL,
  DIV,
  MOD,

  AND,
  OR,
  BITAND,
  BITOR,
  BITXOR,
  LSHIFT,
  RSHIFT,

  EQ,
  NEQ,
  LT,
  GT,
  LEQ,
  GEQ,

  ASSIGN,
  ADD_ASSIGN,
  SUB_ASSIGN,
  MUL_ASSIGN,
  DIV_ASSIGN,
  MOD_ASSIGN,

  AND_ASSIGN,
  OR_ASSIGN,
  XOR_ASSIGN,
  LSHIFT_ASSIGN,
  RSHIFT_ASSIGN,
  COMMA,
};

struct BinaryExpr : Expr {
  BinaryOp op;
  Expr *lhs = nullptr;
  Expr *rhs = nullptr;
};

struct UnaryExpr : Expr {
  UnaryOp op;
  Expr *operand = nullptr;
};

struct TernaryExpr : Expr {
  Expr *cond = nullptr;
  Expr *then = nullptr;
  Expr *els = nullptr;
};

struct CallExpr : Expr {
  Expr *callee = nullptr;
  std::vector<Expr *> args;
};

struct CastExpr : Expr {
  Type *toType = nullptr;
  Expr *operand = nullptr;
};

struct SizeofExpr : Expr {
  bool ofType = false;
  Type *type = nullptr;
  Expr *expr = nullptr;
};

struct MemberExpr : Expr {
  Expr *base = nullptr;
  std::string member;
  bool arrow = false;
};

struct IndexExpr : Expr {
  Expr *base = nullptr;
  Expr *index = nullptr;
};

struct Stmt : Node {};

struct ExprStmt : Stmt {
  Expr *expr = nullptr;
};
struct BreakStmt : Stmt {};
struct ContinueStmt : Stmt {};

struct CompoundStmt : Stmt {
  std::vector<Node *> items;
};

struct IfStmt : Stmt {
  Expr *cond = nullptr;
  Stmt *then = nullptr;
  Stmt *els = nullptr;
};

struct WhileStmt : Stmt {
  Expr *cond = nullptr;
  Stmt *body = nullptr;
};

struct DoWhileStmt : Stmt {
  Stmt *body = nullptr;
  Expr *cond = nullptr;
};

struct ForStmt : Stmt {
  Node *init = nullptr;
  Expr *cond = nullptr;
  Expr *incr = nullptr;
  Stmt *body = nullptr;
};

struct ReturnStmt : Stmt {
  Expr *value = nullptr;
};

struct GotoStmt : Stmt {
  std::string label;
};

struct LabelStmt : Stmt {
  std::string label;
  Stmt *stmt = nullptr;
};

struct SwitchStmt : Stmt {
  Expr *cond = nullptr;
  Stmt *body = nullptr;
};

struct CaseStmt : Stmt {
  Expr *value = nullptr;
  Stmt *stmt = nullptr;
};

struct DefaultStmt : Stmt {
  Stmt *stmt = nullptr;
};

enum class StorageClass { NONE, AUTO, REGISTER, STATIC, EXTERN, TYPEDEF };

struct Decl : Node {
  std::string name;
  Type *type = nullptr;
};

struct VarDecl : Decl {
  Expr *init = nullptr;
  StorageClass sc = StorageClass::NONE;
  bool isGlobal = false;
};

struct ParamDecl : Decl {};

struct FuncDecl : Decl {
  std::vector<ParamDecl *> params;
  CompoundStmt *body = nullptr;
};

struct StructDecl : Decl {
  struct Field {
    std::string name;
    Type *type = nullptr;
    int offset = 0;
  };
  std::vector<Field> fields;
  bool defined = false;
};

struct UnionDecl : Decl {
  struct Field {
    std::string name;
    Type *type = nullptr;
  };
  std::vector<Field> fields;
  bool defined = false;
};

struct EnumDecl : Decl {
  struct Enumerator {
    std::string name;
    long long val;
  };
  std::vector<Enumerator> enumerators;
};

struct TypedefDecl : Decl {
  Type *underlying = nullptr;
};

struct TranslationUnit : Node {
  std::vector<Decl *> decls;
};
