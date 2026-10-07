#include "parser/Parser.hpp"
#include "parser/ASTDump.hpp"
#include <exception>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#define TEST_EXPECT(cond)                                                           \
  do {                                                                              \
    if (!(cond)) {                                                                  \
      std::cerr << __FILE__ << ":" << __LINE__ << " in " << __func__             \
                << " => EXPECT(" #cond ") failed" << std::endl;                   \
      return false;                                                                 \
    }                                                                               \
  } while (0)

struct ParserTestContext {
  LangOptions opts = LangOptions::forc99();
  std::ostringstream diagnosticsOutput;
  DiagnosticEngine diag;
  TypePool types;
  TargetInfo target = makeTarget();
  Lexer lexer;
  Parser parser;
  ParserTestHelper helper;

  explicit ParserTestContext(const std::string &source)
      : diag("test.c", source, diagnosticsOutput, DiagnosticColor::Never),
        lexer(source, "test.c", diag, opts),
        parser(lexer, types, diag, target, opts),
        helper(parser) {}

  static TargetInfo makeTarget() {
    TargetInfo T{};
    T.arch = TargetInfo::X86_64;
    T.sizeofLong = 8;
    T.sizeofPointer = 8;
    T.maxAlign = 16;
    return T;
  }
};

bool parses_simple_int_decl() {
  ParserTestContext ctx("int value;\n");

  Type *base = ctx.helper.parseTypeSpec();
  TEST_EXPECT(base != nullptr);
  std::string name;
  Type *full = ctx.helper.parseDeclarator(base, name);
  TEST_EXPECT(full != nullptr);
  TEST_EXPECT(name == "value");
  TEST_EXPECT(full->kind == Type::INT);
  TEST_EXPECT(ctx.helper.currentKind() == TokenKind::SEMICOLON);
  TEST_EXPECT(!ctx.diag.hasErrors());
  return true;
}

bool parses_unsigned_long_pointer() {
  ParserTestContext ctx("unsigned long **pp;\n");
  Type *base = ctx.helper.parseTypeSpec();
  TEST_EXPECT(base != nullptr);
  std::string name;
  Type *full = ctx.helper.parseDeclarator(base, name);
  TEST_EXPECT(full != nullptr);
  TEST_EXPECT(name == "pp");
  TEST_EXPECT(full->isPointer());
  TEST_EXPECT(full->pointee != nullptr);
  TEST_EXPECT(full->pointee->isPointer());
  TEST_EXPECT(full->pointee->pointee != nullptr);
  TEST_EXPECT(full->pointee->pointee->kind == Type::ULONG);
  TEST_EXPECT(ctx.helper.currentKind() == TokenKind::SEMICOLON);
  TEST_EXPECT(!ctx.diag.hasErrors());
  return true;
}

bool parses_function_pointer_declarator() {
  ParserTestContext ctx("double (*func)(int, float);\n");
  Type *base = ctx.helper.parseTypeSpec();
  TEST_EXPECT(base != nullptr);
  std::string name;
  Type *full = ctx.helper.parseDeclarator(base, name);
  TEST_EXPECT(full != nullptr);
  TEST_EXPECT(name == "func");
  TEST_EXPECT(full->isPointer());
  TEST_EXPECT(full->pointee != nullptr);
  TEST_EXPECT(full->pointee->isFunction());
  TEST_EXPECT(full->pointee->retType == base);
  TEST_EXPECT(full->pointee->params.size() == 2);
  TEST_EXPECT(full->pointee->params[0]->kind == Type::INT);
  TEST_EXPECT(full->pointee->params[1]->kind == Type::FLOAT);
  TEST_EXPECT(!full->pointee->variadic);
  TEST_EXPECT(ctx.helper.currentKind() == TokenKind::SEMICOLON);
  TEST_EXPECT(!ctx.diag.hasErrors());
  return true;
}

bool parses_array_with_constant_size() {
  ParserTestContext ctx("int data[4];\n");
  Type *base = ctx.helper.parseTypeSpec();
  std::string name;
  Type *full = ctx.helper.parseDeclarator(base, name);
  TEST_EXPECT(full != nullptr);
  TEST_EXPECT(full->isArray());
  TEST_EXPECT(full->arraySize == 4);
  TEST_EXPECT(ctx.helper.currentKind() == TokenKind::SEMICOLON);
  return true;
}

bool parses_binary_expression_precedence() {
  ParserTestContext ctx("1 + 2 * 3;\n");
  Expr *expr = ctx.helper.parseExpr();
  TEST_EXPECT(expr != nullptr);
  auto *add = dynamic_cast<BinaryExpr *>(expr);
  TEST_EXPECT(add != nullptr);
  TEST_EXPECT(add->op == BinaryOp::ADD);
  auto *rhs = dynamic_cast<BinaryExpr *>(add->rhs);
  TEST_EXPECT(rhs != nullptr);
  TEST_EXPECT(rhs->op == BinaryOp::MUL);
  TEST_EXPECT(ctx.helper.currentKind() == TokenKind::SEMICOLON);
  return true;
}

bool parses_sizeof_type_expression() {
  ParserTestContext ctx("sizeof(int *);\n");
  Expr *expr = ctx.helper.parseExpr();
  auto *sz = dynamic_cast<SizeofExpr *>(expr);
  TEST_EXPECT(sz != nullptr);
  TEST_EXPECT(sz->ofType);
  TEST_EXPECT(sz->operandType != nullptr);
  TEST_EXPECT(sz->operandType->isPointer());
  TEST_EXPECT(sz->operandType->pointee->kind == Type::INT);
  TEST_EXPECT(ctx.helper.currentKind() == TokenKind::SEMICOLON);
  return true;
}

bool reports_parser_error_with_source() {
  ParserTestContext ctx("1 + ;\n");
  TEST_EXPECT(ctx.helper.parseExpr() == nullptr);
  TEST_EXPECT(ctx.diag.errorCount() == 1);
  TEST_EXPECT(ctx.diagnosticsOutput.str() ==
              "test.c:1:5: error: expected expression\n"
              "    1 | 1 + ;\n"
              "      |     ^\n");
  return true;
}

std::string readTestSource(const char *filename) {
  const std::string path = std::string(CC1_TEST_SOURCE_DIR) + "/" + filename;
  std::ifstream file(path, std::ios::binary);
  if (!file)
    throw std::runtime_error("cannot open parser fixture: " + path);
  std::ostringstream source;
  source << file.rdbuf();
  if (file.bad())
    throw std::runtime_error("cannot read parser fixture: " + path);
  return source.str();
}

bool parses_function_body() {
  ParserTestContext ctx(readTestSource("function_body.c"));
  TranslationUnit *tu = ctx.parser.parse();
  TEST_EXPECT(!ctx.diag.hasErrors());
  TEST_EXPECT(tu != nullptr);
  TEST_EXPECT(tu->decls.size() == 1);

  auto *fn = dynamic_cast<FuncDecl *>(tu->decls[0]);
  TEST_EXPECT(fn != nullptr);
  TEST_EXPECT(fn->name == "main");
  TEST_EXPECT(fn->type != nullptr && fn->type->isFunction());
  TEST_EXPECT(fn->type->retType->kind == Type::INT);
  TEST_EXPECT(fn->type->params.empty());
  TEST_EXPECT(fn->body != nullptr);
  TEST_EXPECT(fn->body->items.size() == 1);

  auto *ret = dynamic_cast<ReturnStmt *>(fn->body->items[0]);
  TEST_EXPECT(ret != nullptr);
  auto *add = dynamic_cast<BinaryExpr *>(ret->value);
  TEST_EXPECT(add != nullptr && add->op == BinaryOp::ADD);
  auto *one = dynamic_cast<IntLitExpr *>(add->lhs);
  TEST_EXPECT(one != nullptr && one->val == 1);
  auto *mul = dynamic_cast<BinaryExpr *>(add->rhs);
  TEST_EXPECT(mul != nullptr && mul->op == BinaryOp::MUL);
  auto *two = dynamic_cast<IntLitExpr *>(mul->lhs);
  auto *three = dynamic_cast<IntLitExpr *>(mul->rhs);
  TEST_EXPECT(two != nullptr && two->val == 2);
  TEST_EXPECT(three != nullptr && three->val == 3);
  TEST_EXPECT(ctx.helper.currentKind() == TokenKind::END_OF_FILE);
  return true;
}

bool parses_empty_function_body() {
  ParserTestContext ctx(readTestSource("empty_function_body.c"));
  TranslationUnit *tu = ctx.parser.parse();
  TEST_EXPECT(!ctx.diag.hasErrors());
  TEST_EXPECT(tu->decls.size() == 1);
  auto *fn = dynamic_cast<FuncDecl *>(tu->decls[0]);
  TEST_EXPECT(fn != nullptr && fn->body != nullptr);
  TEST_EXPECT(fn->body->items.empty());
  TEST_EXPECT(ctx.helper.currentKind() == TokenKind::END_OF_FILE);
  return true;
}

bool parses_void_return() {
  ParserTestContext ctx(readTestSource("void_return.c"));
  TranslationUnit *tu = ctx.parser.parse();
  TEST_EXPECT(!ctx.diag.hasErrors());
  TEST_EXPECT(tu->decls.size() == 1);
  auto *fn = dynamic_cast<FuncDecl *>(tu->decls[0]);
  TEST_EXPECT(fn != nullptr && fn->body != nullptr);
  TEST_EXPECT(fn->body->items.size() == 1);
  auto *ret = dynamic_cast<ReturnStmt *>(fn->body->items[0]);
  TEST_EXPECT(ret != nullptr && ret->value == nullptr);
  return true;
}

bool parses_nested_blocks() {
  ParserTestContext ctx(readTestSource("nested_blocks.c"));
  TranslationUnit *tu = ctx.parser.parse();
  TEST_EXPECT(!ctx.diag.hasErrors());
  TEST_EXPECT(tu->decls.size() == 1);
  auto *fn = dynamic_cast<FuncDecl *>(tu->decls[0]);
  TEST_EXPECT(fn != nullptr && fn->body != nullptr);
  TEST_EXPECT(fn->body->items.size() == 2);
  auto *inner = dynamic_cast<CompoundStmt *>(fn->body->items[0]);
  TEST_EXPECT(inner != nullptr && inner->items.size() == 2);
  auto *empty = dynamic_cast<ExprStmt *>(inner->items[0]);
  TEST_EXPECT(empty != nullptr && empty->expr == nullptr);
  auto *innerReturn = dynamic_cast<ReturnStmt *>(inner->items[1]);
  TEST_EXPECT(innerReturn != nullptr);
  auto *two = dynamic_cast<IntLitExpr *>(innerReturn->value);
  TEST_EXPECT(two != nullptr && two->val == 2);
  auto *outerReturn = dynamic_cast<ReturnStmt *>(fn->body->items[1]);
  TEST_EXPECT(outerReturn != nullptr);
  auto *three = dynamic_cast<IntLitExpr *>(outerReturn->value);
  TEST_EXPECT(three != nullptr && three->val == 3);
  TEST_EXPECT(ctx.helper.currentKind() == TokenKind::END_OF_FILE);
  return true;
}

bool parses_expression_and_empty_statements() {
  ParserTestContext ctx(readTestSource("expression_statements.c"));
  TranslationUnit *tu = ctx.parser.parse();
  TEST_EXPECT(!ctx.diag.hasErrors());
  TEST_EXPECT(tu->decls.size() == 1);
  auto *fn = dynamic_cast<FuncDecl *>(tu->decls[0]);
  TEST_EXPECT(fn != nullptr && fn->body != nullptr);
  TEST_EXPECT(fn->body->items.size() == 3);
  auto *exprStmt = dynamic_cast<ExprStmt *>(fn->body->items[0]);
  TEST_EXPECT(exprStmt != nullptr);
  auto *add = dynamic_cast<BinaryExpr *>(exprStmt->expr);
  TEST_EXPECT(add != nullptr && add->op == BinaryOp::ADD);
  auto *empty = dynamic_cast<ExprStmt *>(fn->body->items[1]);
  TEST_EXPECT(empty != nullptr && empty->expr == nullptr);
  TEST_EXPECT(dynamic_cast<ReturnStmt *>(fn->body->items[2]) != nullptr);
  return true;
}

bool parses_prototype_followed_by_definition() {
  ParserTestContext ctx(readTestSource("prototype_and_definition.c"));
  TranslationUnit *tu = ctx.parser.parse();
  TEST_EXPECT(!ctx.diag.hasErrors());
  TEST_EXPECT(tu->decls.size() == 2);
  auto *prototype = dynamic_cast<FuncDecl *>(tu->decls[0]);
  auto *definition = dynamic_cast<FuncDecl *>(tu->decls[1]);
  TEST_EXPECT(prototype != nullptr && prototype->name == "f");
  TEST_EXPECT(prototype->body == nullptr);
  TEST_EXPECT(definition != nullptr && definition->name == "main");
  TEST_EXPECT(definition->body != nullptr);
  TEST_EXPECT(definition->body->items.size() == 1);
  TEST_EXPECT(ctx.helper.currentKind() == TokenKind::END_OF_FILE);
  return true;
}

bool checks_function_body_error(const std::string &source,
                                const std::string &message,
                                uint32_t line, uint32_t col) {
  ParserTestContext ctx(source);
  ctx.parser.parse();
  TEST_EXPECT(ctx.diag.hasErrors());
  // The parser should stop at the first error, without secondary diagnostics.
  if (ctx.diag.errorCount() != 1)
    std::cerr << ctx.diagnosticsOutput.str();
  TEST_EXPECT(ctx.diag.errorCount() == 1);
  const auto &diagnostic = ctx.diag.diagnostics().front();
  TEST_EXPECT(diagnostic.message == message);
  TEST_EXPECT(diagnostic.line == line);
  TEST_EXPECT(diagnostic.col == col);
  return true;
}

bool rejects_return_without_semicolon() {
  return checks_function_body_error(readTestSource("invalid_missing_semicolon.c"),
                                    "expected ';' after return", 3, 1);
}

bool rejects_unclosed_function_body() {
  return checks_function_body_error(readTestSource("invalid_unclosed_block.c"),
                                    "expected '}' at end of block", 3, 1);
}

bool rejects_incomplete_return_expression() {
  return checks_function_body_error(readTestSource("invalid_return_expression.c"),
                                    "expected expression", 2, 14);
}

bool rejects_malformed_parameter_lists() {
  struct Case {
    const char *source;
    uint32_t col;
  };
  const Case cases[] = {
      {"int broken(,) {}", 12},
      {"int broken(int a,) {}", 18},
      {"int (*broken)(,);", 15},
      {"typedef int (*broken)(,);", 23},
      {"int broken(int (*callback)(,)) {}", 28},
      {"int (*broken(int (*callback)(,)))(int);", 30},
      {"int (broken(,));", 13},
      {"int broken([2]);", 12},
      {"int broken(());", 12},
  };

  for (const auto &test : cases) {
    ParserTestContext ctx(test.source);
    auto *tu = ctx.parser.parse();
    TEST_EXPECT(tu != nullptr);
    TEST_EXPECT(tu->decls.empty());
    TEST_EXPECT(ctx.diag.errorCount() == 1);
    const auto &diagnostic = ctx.diag.diagnostics().front();
    TEST_EXPECT(diagnostic.message == "expected parameter type");
    TEST_EXPECT(diagnostic.line == 1);
    TEST_EXPECT(diagnostic.col == test.col);
  }
  return true;
}

bool rejects_malformed_sizeof_function_type() {
  return checks_function_body_error(
      "int broken() { return sizeof(int (*)(,)); }",
      "expected parameter type", 1, 38);
}

FuncDecl *findFunction(TranslationUnit *tu, const std::string &name) {
  for (auto *decl : tu->decls)
    if (auto *fn = dynamic_cast<FuncDecl *>(decl))
      if (fn->name == name)
        return fn;
  return nullptr;
}

ReturnStmt *firstReturn(FuncDecl *fn) {
  if (!fn || !fn->body || fn->body->items.empty())
    return nullptr;
  return dynamic_cast<ReturnStmt *>(fn->body->items.front());
}

bool parses_two_call_arguments() {
  ParserTestContext ctx(readTestSource("ast_calls.c"));
  auto *tu = ctx.parser.parse();
  TEST_EXPECT(!ctx.diag.hasErrors());
  auto *ret = firstReturn(findFunction(tu, "main"));
  TEST_EXPECT(ret != nullptr);
  auto *call = dynamic_cast<CallExpr *>(ret->value);
  TEST_EXPECT(call != nullptr);
  std::cout << "sum(1, 2): " << call->args.size() << " argument(s), expected 2\n";
  TEST_EXPECT(call->args.size() == 2);
  auto *one = dynamic_cast<IntLitExpr *>(call->args[0]);
  auto *two = dynamic_cast<IntLitExpr *>(call->args[1]);
  TEST_EXPECT(one != nullptr && one->val == 1);
  TEST_EXPECT(two != nullptr && two->val == 2);
  return true;
}

bool preserves_parenthesized_comma_argument() {
  ParserTestContext ctx(readTestSource("ast_comma_argument.c"));
  auto *tu = ctx.parser.parse();
  TEST_EXPECT(!ctx.diag.hasErrors());
  auto *ret = firstReturn(findFunction(tu, "main"));
  TEST_EXPECT(ret != nullptr);
  auto *call = dynamic_cast<CallExpr *>(ret->value);
  TEST_EXPECT(call != nullptr && call->args.size() == 1);
  auto *comma = dynamic_cast<BinaryExpr *>(call->args[0]);
  TEST_EXPECT(comma != nullptr && comma->op == BinaryOp::COMMA);
  auto *one = dynamic_cast<IntLitExpr *>(comma->lhs);
  auto *two = dynamic_cast<IntLitExpr *>(comma->rhs);
  TEST_EXPECT(one != nullptr && one->val == 1);
  TEST_EXPECT(two != nullptr && two->val == 2);
  return true;
}

bool parses_call_argument_boundaries() {
  struct Case { const char *source; size_t count; };
  const Case cases[] = {
      {"empty()", 0},
      {"sum(1, 2, 3)", 3},
      {"sum(a = 1, b = 2)", 2},
      {"sum((1, 2), 3)", 2},
      {"outer(inner(1, 2), 3)", 2},
  };
  for (const auto &test : cases) {
    ParserTestContext ctx(test.source);
    auto *call = dynamic_cast<CallExpr *>(ctx.helper.parseExpr());
    TEST_EXPECT(call != nullptr);
    TEST_EXPECT(!ctx.diag.hasErrors());
    TEST_EXPECT(call->args.size() == test.count);
    TEST_EXPECT(ctx.helper.currentKind() == TokenKind::END_OF_FILE);
  }
  return true;
}

bool preserves_conditional_comma_argument() {
  ParserTestContext ctx("sum(flag ? 1, 2 : 3, 4)");
  auto *call = dynamic_cast<CallExpr *>(ctx.helper.parseExpr());
  TEST_EXPECT(call != nullptr && call->args.size() == 2);
  TEST_EXPECT(!ctx.diag.hasErrors());
  auto *conditional = dynamic_cast<TernaryExpr *>(call->args[0]);
  TEST_EXPECT(conditional != nullptr);
  auto *comma = dynamic_cast<BinaryExpr *>(conditional->then);
  TEST_EXPECT(comma != nullptr && comma->op == BinaryOp::COMMA);
  auto *last = dynamic_cast<IntLitExpr *>(call->args[1]);
  TEST_EXPECT(last != nullptr && last->val == 4);
  TEST_EXPECT(ctx.helper.currentKind() == TokenKind::END_OF_FILE);
  return true;
}

bool rejects_malformed_call_arguments() {
  for (const char *source : {"sum(1,)", "sum(,1)", "sum((1,))",
                             "sum(1 ? : 2, 3)"}) {
    ParserTestContext ctx(source);
    TEST_EXPECT(ctx.helper.parseExpr() == nullptr);
    TEST_EXPECT(ctx.diag.errorCount() == 1);
    TEST_EXPECT(ctx.diag.diagnostics().front().message == "expected expression");
  }
  ParserTestContext missingSeparator("sum(1 2)");
  TEST_EXPECT(missingSeparator.helper.parseExpr() == nullptr);
  TEST_EXPECT(missingSeparator.diag.errorCount() == 1);
  TEST_EXPECT(missingSeparator.diag.diagnostics().front().message ==
              "expected ')' in call expression");
  return true;
}

bool preserves_function_parameter_names() {
  ParserTestContext ctx(readTestSource("ast_assignments.c"));
  auto *tu = ctx.parser.parse();
  TEST_EXPECT(!ctx.diag.hasErrors());
  auto *fn = findFunction(tu, "assign");
  TEST_EXPECT(fn != nullptr && fn->type->params.size() == 3);
  std::cout << "assign: " << fn->params.size() << " named parameter(s), expected 3\n";
  TEST_EXPECT(fn->params.size() == 3);
  TEST_EXPECT(fn->params[0]->name == "a");
  TEST_EXPECT(fn->params[1]->name == "b");
  TEST_EXPECT(fn->params[2]->name == "c");
  return true;
}

bool preserves_nested_function_parameters() {
  ParserTestContext ctx(
      "int apply(int (*callback)(int nested), int tail);\n"
      "int prototype(int, float named);\n"
      "int (*factory(int seed))(int value);\n"
      "int variadic(int first, ...);\n"
      "int empty(void);\n");
  auto *tu = ctx.parser.parse();
  TEST_EXPECT(!ctx.diag.hasErrors());
  TEST_EXPECT(tu->decls.size() == 5);
  auto *apply = findFunction(tu, "apply");
  TEST_EXPECT(apply != nullptr && apply->params.size() == 2);
  TEST_EXPECT(apply->params[0]->name == "callback");
  TEST_EXPECT(apply->params[1]->name == "tail");
  TEST_EXPECT(apply->params[0]->type == apply->type->params[0]);
  TEST_EXPECT(apply->params[0]->type->isPointer());
  TEST_EXPECT(apply->params[0]->type->pointee->isFunction());
  TEST_EXPECT(apply->params[0]->loc.line == 1 && apply->params[0]->loc.col == 11);
  auto *prototype = findFunction(tu, "prototype");
  TEST_EXPECT(prototype != nullptr && prototype->params.size() == 2);
  TEST_EXPECT(prototype->params[0]->name.empty());
  TEST_EXPECT(prototype->params[1]->name == "named");
  auto *factory = findFunction(tu, "factory");
  TEST_EXPECT(factory != nullptr && factory->params.size() == 1);
  TEST_EXPECT(factory->params[0]->name == "seed");
  TEST_EXPECT(factory->type->retType->isPointer());
  TEST_EXPECT(factory->type->retType->pointee->isFunction());
  auto *variadic = findFunction(tu, "variadic");
  TEST_EXPECT(variadic != nullptr && variadic->type->variadic);
  TEST_EXPECT(variadic->params.size() == 1 && variadic->params[0]->name == "first");
  auto *empty = findFunction(tu, "empty");
  TEST_EXPECT(empty != nullptr && empty->params.empty());
  return true;
}

bool preserves_tag_declarations() {
  ParserTestContext ctx(readTestSource("mixed_structs.c"));
  auto *tu = ctx.parser.parse();
  TEST_EXPECT(!ctx.diag.hasErrors());
  std::cout << "mixed_structs: " << tu->decls.size() << " declaration(s), expected 6\n";
  TEST_EXPECT(tu->decls.size() == 6);
  TEST_EXPECT(dynamic_cast<StructDecl *>(tu->decls[0]) != nullptr);
  TEST_EXPECT(dynamic_cast<UnionDecl *>(tu->decls[1]) != nullptr);
  TEST_EXPECT(dynamic_cast<EnumDecl *>(tu->decls[2]) != nullptr);
  TEST_EXPECT(tu->decls[0]->name == "Foo" && tu->decls[0]->type->kind == Type::STRUCT);
  TEST_EXPECT(tu->decls[1]->name == "Bar" && tu->decls[1]->type->kind == Type::UNION);
  TEST_EXPECT(tu->decls[2]->name == "Color" && tu->decls[2]->type->kind == Type::ENUM);
  TEST_EXPECT(tu->decls[3]->name == "foo_ptr");
  TEST_EXPECT(tu->decls[4]->name == "value");
  TEST_EXPECT(tu->decls[5]->name == "shade");
  return true;
}

bool decodes_string_literal_escape() {
  ParserTestContext ctx(readTestSource("ast_literals.c"));
  auto *tu = ctx.parser.parse();
  TEST_EXPECT(!ctx.diag.hasErrors());
  auto *ret = firstReturn(findFunction(tu, "text"));
  TEST_EXPECT(ret != nullptr);
  auto *literal = dynamic_cast<StringLitExpr *>(ret->value);
  TEST_EXPECT(literal != nullptr);
  std::cout << "hello\\n: " << literal->val.size() << " decoded byte(s), expected 6\n";
  TEST_EXPECT(literal->val == "hello\n");
  return true;
}

bool dumps_parsed_expression_nodes() {
  struct Case { const char *fixture; const char *node; };
  const Case cases[] = {
      {"ast_unary.c", "UnaryExpr"},
      {"ast_conditional.c", "TernaryExpr"},
      {"ast_calls.c", "CallExpr"},
      {"ast_index.c", "IndexExpr"},
      {"ast_member.c", "MemberExpr"},
      {"ast_sizeof.c", "SizeofExpr"},
      {"ast_literals.c", "CharLitExpr"},
      {"ast_literals.c", "StringLitExpr"},
  };
  bool complete = true;
  for (const auto &test : cases) {
    ParserTestContext ctx(readTestSource(test.fixture));
    auto *tu = ctx.parser.parse();
    TEST_EXPECT(!ctx.diag.hasErrors());
    std::ostringstream output;
    dumpAST(tu, output);
    if (output.str().find(test.node) == std::string::npos ||
        output.str().find("UnsupportedNode") != std::string::npos) {
      std::cerr << test.fixture << ": dump is missing " << test.node << '\n';
      complete = false;
    }
  }
  return complete;
}

bool parses_local_declarations_and_typedef_scopes() {
  ParserTestContext ctx(
      "typedef int T; int f(void) { int x = 1 + 2; { typedef short T; T y = 3; "
      "} int T = 4; return sizeof(T); } T global;");
  auto *tu = ctx.parser.parse();
  TEST_EXPECT(!ctx.diag.hasErrors());
  auto *fn = findFunction(tu, "f");
  TEST_EXPECT(fn && fn->body->items.size() == 4);
  auto *x = dynamic_cast<VarDecl *>(fn->body->items[0]);
  TEST_EXPECT(x && !x->isGlobal && x->name == "x");
  TEST_EXPECT(dynamic_cast<BinaryExpr *>(x->init));
  auto *nested = dynamic_cast<CompoundStmt *>(fn->body->items[1]);
  TEST_EXPECT(nested && nested->items.size() == 2);
  auto *y = dynamic_cast<VarDecl *>(nested->items[1]);
  TEST_EXPECT(y && y->type->kind == Type::SHORT && !y->isGlobal);
  auto *ret = dynamic_cast<ReturnStmt *>(fn->body->items[3]);
  auto *size = ret ? dynamic_cast<SizeofExpr *>(ret->value) : nullptr;
  TEST_EXPECT(size && !size->ofType && dynamic_cast<IdentExpr *>(size->expr));
  auto *global = dynamic_cast<VarDecl *>(tu->decls.back());
  TEST_EXPECT(global && global->isGlobal && global->type->kind == Type::INT);
  return true;
}

bool diagnoses_invalid_local_initializers() {
  ParserTestContext ctx("int f(void) { int x = ; return 0; }");
  ctx.parser.parse();
  TEST_EXPECT(ctx.diag.errorCount() == 1);
  TEST_EXPECT(ctx.diag.diagnostics().front().message == "expected expression");
  return true;
}

int main() {
  struct TestEntry {
    const char *name;
    bool (*fn)();
  };

  const std::vector<TestEntry> tests = {
      {"parses_simple_int_decl", parses_simple_int_decl},
      {"parses_local_declarations_and_typedef_scopes", parses_local_declarations_and_typedef_scopes},
      {"diagnoses_invalid_local_initializers", diagnoses_invalid_local_initializers},
      {"parses_unsigned_long_pointer", parses_unsigned_long_pointer},
      {"parses_function_pointer_declarator", parses_function_pointer_declarator},
      {"parses_array_with_constant_size", parses_array_with_constant_size},
      {"parses_binary_expression_precedence", parses_binary_expression_precedence},
      {"parses_sizeof_type_expression", parses_sizeof_type_expression},
      {"reports_parser_error_with_source", reports_parser_error_with_source},
      {"parses_function_body", parses_function_body},
      {"parses_empty_function_body", parses_empty_function_body},
      {"parses_void_return", parses_void_return},
      {"parses_nested_blocks", parses_nested_blocks},
      {"parses_expression_and_empty_statements", parses_expression_and_empty_statements},
      {"parses_prototype_followed_by_definition", parses_prototype_followed_by_definition},
      {"rejects_return_without_semicolon", rejects_return_without_semicolon},
      {"rejects_unclosed_function_body", rejects_unclosed_function_body},
      {"rejects_incomplete_return_expression", rejects_incomplete_return_expression},
      {"rejects_malformed_parameter_lists", rejects_malformed_parameter_lists},
      {"rejects_malformed_sizeof_function_type", rejects_malformed_sizeof_function_type},
      {"parses_two_call_arguments", parses_two_call_arguments},
      {"preserves_parenthesized_comma_argument", preserves_parenthesized_comma_argument},
      {"parses_call_argument_boundaries", parses_call_argument_boundaries},
      {"preserves_conditional_comma_argument", preserves_conditional_comma_argument},
      {"rejects_malformed_call_arguments", rejects_malformed_call_arguments},
      {"preserves_function_parameter_names", preserves_function_parameter_names},
      {"preserves_nested_function_parameters", preserves_nested_function_parameters},
      {"preserves_tag_declarations", preserves_tag_declarations},
      {"decodes_string_literal_escape", decodes_string_literal_escape},
      {"dumps_parsed_expression_nodes", dumps_parsed_expression_nodes},
  };

  int failed = 0;
  for (const auto &test : tests) {
    bool ok = false;
    try {
      ok = test.fn();
    } catch (const std::exception &ex) {
      ok = false;
      std::cerr << test.name << " threw exception: " << ex.what() << std::endl;
    } catch (...) {
      ok = false;
      std::cerr << test.name << " threw unknown exception" << std::endl;
    }

    if (ok) {
      std::cout << "[PASSED] " << test.name << std::endl;
    } else {
      std::cerr << "[FAILED] " << test.name << std::endl;
      ++failed;
    }
  }

  return failed == 0 ? 0 : 1;
}
