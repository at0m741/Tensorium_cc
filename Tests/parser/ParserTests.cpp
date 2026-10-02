#include "parser/Parser.hpp"
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
        lexer(source, "test.c", diag),
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
  TEST_EXPECT(sz->type != nullptr);
  TEST_EXPECT(sz->type->isPointer());
  TEST_EXPECT(sz->type->pointee->kind == Type::INT);
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

int main() {
  struct TestEntry {
    const char *name;
    bool (*fn)();
  };

  const std::vector<TestEntry> tests = {
      {"parses_simple_int_decl", parses_simple_int_decl},
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
