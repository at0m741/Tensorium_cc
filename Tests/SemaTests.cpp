#include "cc1/LangOptions.hpp"
#include "lexer/Lexer.hpp"
#include "parser/ASTDump.hpp"
#include "parser/Parser.hpp"
#include "preprocessor/Preprocessor.hpp"
#include "sema/Sema.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>

#define EXPECT(condition)                                                      \
  do {                                                                         \
    if (!(condition))                                                          \
      throw std::runtime_error(#condition);                                    \
  } while (false)

struct Context {
  std::string source;
  std::ostringstream output;
  DiagnosticEngine diag;
  LangOptions options = LangOptions::forc99();
  TargetInfo target;
  TypePool types;
  Lexer lexer;
  Parser parser;
  std::unique_ptr<TranslationUnit> unit;
  Sema sema;
  explicit Context(std::string text, TargetInfo t = TargetInfo::x86_64())
      : source(std::move(text)),
        diag("sema.c", source, output, DiagnosticColor::Never), target(t),
        lexer(source, "sema.c", diag, options),
        parser(lexer, types, diag, target, options), unit(parser.parse()),
        sema(types, diag, target) {}
  bool analyze() {
    if (diag.hasErrors())
      throw std::runtime_error(source + "\n" + output.str());
    return sema.analyze(*unit);
  }
  FuncDecl *function(const std::string &name) {
    for (auto *d : unit->decls)
      if (auto *f = dynamic_cast<FuncDecl *>(d))
        if (f->name == name && f->body)
          return f;
    throw std::runtime_error("missing function");
  }
  Expr *returned(const std::string &name) {
    auto *f = function(name);
    for (auto *item : f->body->items)
      if (auto *r = dynamic_cast<ReturnStmt *>(item))
        return r->value;
    throw std::runtime_error("missing return");
  }
};

void accepts_supported_programs() {
  const char *cases[] = {
      "int g; int f(int x) { x += g; { x++; } return x; }",
      "int f(int); int f(int x) { return f(x); }",
      "int f(void); int f(void); int f(void) { return 1; }",
      "int f(); int f(int); int f(int x) { return x; }",
      "int f(); int g(void) { return f(1, 2.0); }",
      "int f(int a[], int callback(int)); int add(int x) { return x; } int "
      "data[4]; int g(void) { return f(data, add); }",
      "int a[2][3]; int f(void) { return a[1][2]; }",
      "int f(int a[2][3]) { return a[1][2]; }",
      "const int *p; int *q; const int *f(int c) { return c ? p : q; }",
      "int *p; int *f(int c) { return c ? p : 0; }",
      "int *p; void *q; int f(void) { return p == q; }",
      "int *p; int *f(int n) { p += n; return n + p; }",
      "long f(int *p, int *q) { return p - q; }",
      "void f(void) { return; } void g(int c) { c ? f() : f(); return; }",
      "char *f(void) { return \"hello\"; }",
      "int f(int x, ...); int g(float x, char y) { return f(1, x, y); }",
      "typedef int I; const I x; I y; int f(I p) { return p + y; }",
      "struct S; struct S *p; int f(void) { return sizeof p; }",
      "int tag; struct tag; struct tag *p; int f(void) { return tag; }",
      "int a[]; extern int a[4]; int f(void) { return sizeof a; }",
      "int a[];",
      "extern int a[]; extern struct S s;",
      "int f(int (*callback)(int), int x) { return callback(x); }",
      "int (*factory(int seed))(int value); int g(int x) { return "
      "factory(x)(x); }",
      "int f(int x) { return +x; }",
      "int (*p)(int); int f(void) { return (*p)(1); }",
      "int f(int x) { int y = x + 1; { int x = 2; y = x; } return y; }",
      "typedef int T; int f(void) { { typedef short T; T x = 1; } T x = 2; "
      "return x; }",
      "typedef int T; int f(int *T) { return sizeof(T); } T value;",
  };
  for (auto *source : cases) {
    Context c(source);
    if (!c.analyze())
      throw std::runtime_error(std::string(source) + "\n" + c.output.str());
  }
}

void rejects_semantic_errors() {
  struct Case {
    const char *source;
    const char *message;
  };
  const Case cases[] = {
      {"int f(void) { return missing; }", "undeclared identifier"},
      {"int f(void) { int x = missing; return x; }", "undeclared identifier"},
      {"int f(int x) { int x = 1; return x; }", "conflicting declaration"},
      {"int f(void) { { int x = 1; } return x; }", "undeclared identifier"},
      {"int f(void) { int *p = 1; return 0; }", "incompatible initializer"},
      {"int f(void) { const int x = 1; x = 2; return x; }",
       "modifiable lvalue"},
      {"int f(void) { return later(); } int later(void);",
       "undeclared identifier"},
      {"int f(int x) { return x; } int g(void) { return x; }",
       "undeclared identifier"},
      {"int f(int x, int x) { return x; }", "conflicting declaration"},
      {"int f(int x, int x);", "conflicting declaration"},
      {"int f(int); int f() {}", "conflicting declaration"},
      {"int f() {} int f(); int f(int);", "conflicting declaration"},
      {"int f(int p[0]);", "array size must be positive"},
      {"int f(int) { return 1; }", "parameter name required"},
      {"int f(void) { return; }", "non-void function must return a value"},
      {"void f(void) { return 1; }", "void function cannot return a value"},
      {"int f(void); int g(void) { return f(1); }",
       "incorrect number of arguments"},
      {"int f(int); int g(void) { return f(); }",
       "incorrect number of arguments"},
      {"int f(int *); int g(void) { return f(1); }",
       "incompatible argument type"},
      {"int f(void) { return 1(2); }", "not a function"},
      {"int f(void) { 1 = 2; return 0; }", "modifiable lvalue"},
      {"const int x; int f(void) { x = 2; return x; }", "modifiable lvalue"},
      {"int a[2]; int f(void) { a = 0; return 0; }", "modifiable lvalue"},
      {"typedef int A[2]; const A a; int f(void) { a[0] = 1; return 0; }",
       "modifiable lvalue"},
      {"int f(void) { ++1; return 0; }", "increment/decrement"},
      {"int f(void) { return &1; }", "address-of"},
      {"int f(int x) { return *x; }", "dereference"},
      {"int f(float x) { return x % 2; }", "invalid operands"},
      {"int f(double x) { return ~x; }", "invalid operand"},
      {"int f(float x) { return x << 2; }", "invalid operands"},
      {"int *p; int *q; int f(void) { return p + q; }", "invalid operands"},
      {"int f(void *p) { p++; return 0; }", "increment/decrement"},
      {"int f(void *p) { return p[1]; }", "indexing"},
      {"int f(int *p) { return p[1.0]; }", "indexing"},
      {"int f(void) { return sizeof(void); }", "complete object type"},
      {"int f(void) { return sizeof(int (int)); }", "complete object type"},
      {"struct S; int f(void) { return sizeof(struct S); }",
       "complete object type"},
      {"int f(void) { return sizeof missing; }", "undeclared identifier"},
      {"struct S *p; int f(void) { return p->field; }",
       "complete struct or union"},
      {"int f(int x) { return x.member; }", "complete struct or union"},
      {"int f(void); double f(void);", "conflicting declaration"},
      {"int f(void) {} int f(void) {}", "redefinition"},
      {"int f(void); int f;", "conflicting declaration"},
      {"int x; float x;", "conflicting declaration"},
      {"void x;", "invalid object type"},
      {"int a[0];", "array size must be positive"},
      {"void a[2];", "array element type"},
      {"int f()[2];", "function cannot return"},
      {"int f(void x);", "parameter cannot have void"},
      {"int f(...);", "variadic function requires"},
      {"struct S; union S;", "conflicting tag kind"},
      {"struct S s;", "invalid object type"},
      {"int *p; const int *q; int f(void) { p = q; return 0; }",
       "incompatible assignment"},
      {"int **p; const int **q; int f(void) { q = p; return 0; }",
       "incompatible assignment"},
      {"int *p; double *q; int f(void) { return p == q; }", "invalid operands"},
      {"int f(int x) { return x ? 1 : \"text\"; }", "incompatible conditional"},
      {"int *f(int x) { return x; }", "incompatible return"},
      {"int f(int *p) { return p; }", "incompatible return"},
      {"int (*p)(int); int f(void) { return p < p; }", "invalid operands"},
      {"auto int x;", "invalid storage class"},
      {"int f(void) { return 1uu; }", "invalid integer literal suffix"},
      {"int f(void) { return 1.0ff; }", "invalid floating literal suffix"},
      {"int f(void) { return 1lL; }", "invalid integer literal suffix"},
  };
  for (auto &test : cases) {
    Context c(test.source);
    EXPECT(!c.analyze());
    if (c.output.str().find(test.message) == std::string::npos)
      throw std::runtime_error(std::string(test.source) + "\n" +
                               c.output.str());
  }
}

void inspects_conversion_tree() {
  Context c("double f(short x, float y) { return x + y; } int global; int "
            "g(int global) { return global; }");
  EXPECT(c.analyze());
  auto *retCast = dynamic_cast<ImplicitCastExpr *>(c.returned("f"));
  EXPECT(retCast && retCast->type->kind == Type::DOUBLE);
  auto *sum = dynamic_cast<BinaryExpr *>(retCast->operand);
  EXPECT(sum && sum->type->kind == Type::FLOAT);
  auto *arithmetic = dynamic_cast<ImplicitCastExpr *>(sum->lhs);
  EXPECT(arithmetic && arithmetic->kind == ImplicitCastKind::Arithmetic);
  auto *load = dynamic_cast<ImplicitCastExpr *>(arithmetic->operand);
  EXPECT(load && load->kind == ImplicitCastKind::LValueToRValue);
  auto *id = dynamic_cast<IdentExpr *>(load->operand);
  EXPECT(id && id->decl == c.function("f")->params[0] && id->isLval);
  auto *gload = dynamic_cast<ImplicitCastExpr *>(c.returned("g"));
  auto *gid = dynamic_cast<IdentExpr *>(gload->operand);
  EXPECT(gid->decl == c.function("g")->params[0]);
  std::ostringstream dump;
  dumpAST(c.unit.get(), dump, 0, true);
  EXPECT(dump.str().find("<untyped>") == std::string::npos);
  EXPECT(dump.str().find("[short, lvalue] IdentExpr x") != std::string::npos);
}

void preserves_sizeof_operand_and_target_types() {
  for (auto target :
       {TargetInfo::i386(), TargetInfo::x86_64(), TargetInfo::aarch64()}) {
    Context c("unsigned long f(void) { return sizeof(\"abc\"); }", target);
    EXPECT(c.analyze());
    auto *size = dynamic_cast<SizeofExpr *>(c.returned("f"));
    EXPECT(size && size->type->kind == Type::ULONG);
    EXPECT(size->operandType->isArray() && size->operandType->arraySize == 4);
    EXPECT(dynamic_cast<StringLitExpr *>(size->expr) != nullptr);
  }
  Context c("int a[2][3]; unsigned long f(void) { return sizeof a; }");
  EXPECT(c.analyze());
  auto *size = dynamic_cast<SizeofExpr *>(c.returned("f"));
  EXPECT(size->operandType->arraySize == 2);
  EXPECT(size->operandType->elemType->arraySize == 3);
}

void records_call_and_compound_conversions() {
  Context c("int f(int first, ...); int g(float x, short y) { return f(1, x, "
            "y); } int h(short x, double y) { x += y; return x; }");
  EXPECT(c.analyze());
  auto *call = dynamic_cast<CallExpr *>(c.returned("g"));
  EXPECT(call && call->type->kind == Type::INT);
  EXPECT(dynamic_cast<ImplicitCastExpr *>(call->callee)->kind ==
         ImplicitCastKind::FunctionToPointer);
  for (size_t i = 1; i < call->args.size(); ++i)
    EXPECT(dynamic_cast<ImplicitCastExpr *>(call->args[i])->kind ==
           ImplicitCastKind::DefaultArgumentPromotion);
  EXPECT(call->args[1]->type->kind == Type::DOUBLE);
  EXPECT(call->args[2]->type->kind == Type::INT);
  auto *stmt = dynamic_cast<ExprStmt *>(c.function("h")->body->items[0]);
  auto *assign = dynamic_cast<BinaryExpr *>(stmt->expr);
  EXPECT(assign && assign->computationType->kind == Type::DOUBLE);
  EXPECT(assign->type->kind == Type::SHORT && !assign->isLval);
  EXPECT(assign->lhs->isLval && dynamic_cast<IdentExpr *>(assign->lhs));
}

void arithmetic_respects_target() {
  for (auto target : {TargetInfo::i386(), TargetInfo::x86_64()}) {
    Context c("long long f(long x, unsigned int y) { x + y; return x << y; }",
              target);
    EXPECT(c.analyze());
    auto *stmt = dynamic_cast<ExprStmt *>(c.function("f")->body->items[0]);
    EXPECT(stmt->expr->type->kind ==
           (target.sizeofLong == 8 ? Type::LONG : Type::ULONG));
    auto *cast = dynamic_cast<ImplicitCastExpr *>(c.returned("f"));
    EXPECT(cast && cast->operand->type->kind == Type::LONG);
  }
}

void keeps_typedef_qualifiers_separate() {
  Context c("typedef int I; const I x; I y; int f(void) { y = 1; return y; }");
  EXPECT(c.analyze());
  EXPECT(c.unit->decls[1]->type->quals.isConst);
  EXPECT(!c.unit->decls[0]->type->quals.isConst);
  EXPECT(!c.unit->decls[2]->type->quals.isConst);
}

void literal_types_respect_suffix_and_target() {
  for (auto target : {TargetInfo::i386(), TargetInfo::x86_64()}) {
    Context c("void f(void) { 1U; 1L; 1ULL; 1.0f; 1.0L; 0xffffffff; "
              "4294967295; return; }",
              target);
    EXPECT(c.analyze());
    const Type::Kind expected[] = {Type::UINT,
                                   Type::LONG,
                                   Type::ULONGLONG,
                                   Type::FLOAT,
                                   Type::LONGDOUBLE,
                                   Type::UINT,
                                   target.sizeofLong == 8 ? Type::LONG
                                                          : Type::LONGLONG};
    for (size_t i = 0; i < 7; ++i) {
      auto *stmt = dynamic_cast<ExprStmt *>(c.function("f")->body->items[i]);
      EXPECT(stmt && stmt->expr->type->kind == expected[i]);
    }
  }
}

void preserves_canonical_declarations_and_pointer_conversions() {
  Context c("int f(int); int f(int x) { return f(x); } int a[4]; int *p; const "
            "int *q; const int *g(void) { p == q; return a; }");
  EXPECT(c.analyze());
  EXPECT(c.function("f")->canonicalDecl == c.unit->decls[0]);
  auto *call = dynamic_cast<CallExpr *>(c.returned("f"));
  auto *decay = dynamic_cast<ImplicitCastExpr *>(call->callee);
  auto *id = dynamic_cast<IdentExpr *>(decay->operand);
  EXPECT(id->decl == c.function("f"));
  auto *stmt = dynamic_cast<ExprStmt *>(c.function("g")->body->items[0]);
  auto *eq = dynamic_cast<BinaryExpr *>(stmt->expr);
  EXPECT(eq && eq->lhs->type->pointee->quals.isConst);
  auto *pointerCast = dynamic_cast<ImplicitCastExpr *>(c.returned("g"));
  EXPECT(pointerCast && pointerCast->kind == ImplicitCastKind::Pointer);
  EXPECT(dynamic_cast<ImplicitCastExpr *>(pointerCast->operand)->kind ==
         ImplicitCastKind::ArrayToPointer);
}

void avoids_cascading_diagnostics() {
  Context c("int f(void) { return missing + 1; }");
  EXPECT(!c.analyze());
  EXPECT(c.diag.errorCount() == 1);
  EXPECT(c.diag.diagnostics()[0].line == 1 &&
         c.diag.diagnostics()[0].col == 22);
}

void accepts_for_loops() {
  const char *cases[] = {
      "int f(int n) { int r = 0; for (int i = 0; i < n; i++) r += i; return r; "
      "}",
      "int f(int n) { for (n = 0; n < 4; ++n) ; return n; }",
      "int f(void) { for (;;) break; return 0; }",
      "int f(int n) { for (;n;) { --n; continue; } return n; }",
      "int f(int n) { for (;;n--) if (n == 0) break; return n; }",
      "int f(int n) { for (int i = 0;;) { if (i == n) break; ++i; } return n; "
      "}",
      "int f(int *p) { for (;p;) break; return 0; }",
      "void step(void); void f(int n) { for (;n;step()) --n; }",
      "int f(void) { for (register int i = 0; i < 2; ++i) ; return 0; }",
      "int f(void) { for (int i = 0; i < 2; ++i) { int i = 3; } return 0; }",
      "typedef int T; int f(void) { for (int T = 0; T < 2; ++T) ; T x = 0; "
      "return x; }",
      ("int f(void) { for (int i = 0; i < 2; ++i) { while (i) { break; } "
       "for (int i = 0; i < 3; ++i) continue; continue; } return 0; }"),
  };
  for (const char *source : cases) {
    Context c(source);
    if (!c.analyze())
      throw std::runtime_error(std::string(source) + "\n" + c.output.str());
  }
}

void resolves_for_scope_and_conversions() {
  Context c("int f(int n) { int i = 9; for (int i = 0; i < n; i = i + 1) "
            "i = i + 2; return i; }");
  EXPECT(c.analyze());
  auto *fn = c.function("f");
  auto *outer = dynamic_cast<VarDecl *>(fn->body->items[0]);
  auto *loop = dynamic_cast<ForStmt *>(fn->body->items[1]);
  EXPECT(outer && loop);
  auto *inner = dynamic_cast<VarDecl *>(loop->init);
  auto *condition = dynamic_cast<BinaryExpr *>(loop->cond);
  EXPECT(inner && condition && condition->type->kind == Type::INT);
  auto *loaded = dynamic_cast<ImplicitCastExpr *>(condition->lhs);
  EXPECT(loaded && loaded->kind == ImplicitCastKind::LValueToRValue);
  auto *ref = dynamic_cast<IdentExpr *>(loaded->operand);
  EXPECT(ref && ref->decl == inner);
  auto *increment = dynamic_cast<BinaryExpr *>(loop->incr);
  EXPECT(increment && increment->lhs->isLval);
  EXPECT(dynamic_cast<IdentExpr *>(increment->lhs)->decl == inner);
  auto *body = dynamic_cast<ExprStmt *>(loop->body);
  auto *assignment = body ? dynamic_cast<BinaryExpr *>(body->expr) : nullptr;
  EXPECT(assignment &&
         dynamic_cast<IdentExpr *>(assignment->lhs)->decl == inner);
  auto *returned = dynamic_cast<ImplicitCastExpr *>(c.returned("f"));
  EXPECT(returned &&
         dynamic_cast<IdentExpr *>(returned->operand)->decl == outer);
}

void rejects_invalid_for_semantics() {
  struct Case {
    const char *source;
    const char *message;
  };
  const Case cases[] = {
      {"int f(void) { for (int i = 0; i < 2; ++i) ; return i; }",
       "undeclared identifier 'i'"},
      {"int f(void) { for (;;x++) { int x = 0; } return 0; }",
       "undeclared identifier 'x'"},
      {"void step(void); int f(void) { for (;step();) ; return 0; }",
       "for requires a scalar condition"},
      {"int f(void) { for (static int i = 0;;) ; return 0; }",
       "for initializer declaration"},
      {"int f(void) { for (extern int i;;) ; return 0; }",
       "for initializer declaration"},
      {"int f(void) { for (typedef int T;;) ; return 0; }",
       "for initializer declaration"},
      {"int f(void) { for (int g(void);;) ; return 0; }",
       "for initializer declaration"},
      {"int f(void) { for (;;1++) ; return 0; }", "modifiable"},
      {"int f(void) { for (;;) break; continue; return 0; }",
       "continue outside loop"},
      {"int f(void) { for (;;) break; break; return 0; }",
       "break outside loop"},
  };
  for (const auto &test : cases) {
    Context c(test.source);
    EXPECT(!c.analyze());
    EXPECT(c.output.str().find(test.message) != std::string::npos);
  }
}

int main() {
  struct Test {
    const char *name;
    void (*run)();
  };
  const Test tests[] = {
      {"accepts_for_loops", accepts_for_loops},
      {"resolves_for_scope_and_conversions", resolves_for_scope_and_conversions},
      {"rejects_invalid_for_semantics", rejects_invalid_for_semantics},
      {"accepts_supported_programs", accepts_supported_programs},
      {"rejects_semantic_errors", rejects_semantic_errors},
      {"inspects_conversion_tree", inspects_conversion_tree},
      {"preserves_sizeof_operand_and_target_types",
       preserves_sizeof_operand_and_target_types},
      {"records_call_and_compound_conversions",
       records_call_and_compound_conversions},
      {"arithmetic_respects_target", arithmetic_respects_target},
      {"keeps_typedef_qualifiers_separate", keeps_typedef_qualifiers_separate},
      {"literal_types_respect_suffix_and_target",
       literal_types_respect_suffix_and_target},
      {"preserves_canonical_declarations_and_pointer_conversions",
       preserves_canonical_declarations_and_pointer_conversions},
      {"avoids_cascading_diagnostics", avoids_cascading_diagnostics},
  };
  int failed = 0;
  for (auto &test : tests) {
    try {
      test.run();
      std::cout << "[PASSED] " << test.name << '\n';
    } catch (const std::exception &e) {
      ++failed;
      std::cerr << "[FAILED] " << test.name << ": " << e.what() << '\n';
    }
  }
  return failed ? 1 : 0;
}
