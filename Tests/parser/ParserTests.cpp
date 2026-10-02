#include "parser/Parser.hpp"
#include <exception>
#include <iostream>
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
  Diagnostic diag;
  TypePool types;
  TargetInfo target = makeTarget();
  Lexer lexer;
  Parser parser;
  ParserTestHelper helper;

  explicit ParserTestContext(const std::string &source)
      : lexer(source, "test.c"), parser(lexer, types, diag, target, opts),
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
