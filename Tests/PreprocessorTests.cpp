#include "preprocessor/Preprocessor.hpp"
#include "parser/Parser.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace {
void check(bool condition, const std::string &message) {
  if (!condition) throw std::runtime_error(message);
}

struct Context {
  std::ostringstream output;
  DiagnosticEngine diag;
  SourceManager sources;
  const SourceFile &file;
  Preprocessor pp;
  Context(const std::string &source, const std::string &filename = "test.c",
          std::vector<std::string> directories = {})
      : diag(filename, source, output, DiagnosticColor::Never), sources(diag),
        file(sources.add(filename, source)),
        pp(sources, file, diag, LangOptions::forc99(), std::move(directories)) {}

  std::string spelling() {
    std::string result;
    for (size_t i = 0; i < 10000; ++i) {
      const auto token = pp.next();
      if (token.isEof() || token.isError()) return result;
      if (!result.empty()) result += ' ';
      result += token.text;
    }
    throw std::runtime_error("token stream did not terminate");
  }
};

void expect(const std::string &source, const std::string &spelling) {
  Context ctx(source);
  const auto result = ctx.spelling();
  check(!ctx.diag.hasErrors(), ctx.output.str());
  check(result == spelling, "incorrect expansion: " + result + "\n" + source);
}

void macros() {
  expect("#define A B\n#define B (1 + 2)\nint value = A;\n", "int value = ( 1 + 2 ) ;");
  expect("#define NAME first\nNAME\n#undef NAME\n#define NAME second\nNAME\n",
         "first second");
  expect("#define EMPTY\nint EMPTY value;\n", "int value ;");
  expect("#define int long\nint value;\n", "long value ;");
  expect("#define X X\nX X\n", "X X");
  expect("#define A B\n#define B A\nA B A\n", "A B A");
  expect("#define X (4 + X)\nX\n", "( 4 + X )");
  expect("#define A (1)\n#define A (1)\nA\n", "( 1 )");
  expect("#define A 1\n#define A /* equivalent */ 1\nA\n", "1");
  Context strings("#define NAME expanded\n\"NAME\" 'N' NAME\n");
  const auto string = strings.pp.next();
  check(string.kind == TokenKind::STRING_LIT && string.str_val == "NAME",
        "macro was expanded inside a string");
  check(strings.pp.next().kind == TokenKind::CHAR_LIT, "character literal changed");
  check(strings.pp.next().text == "expanded", "ordinary macro was not expanded");
}

void conditions() {
  struct Case { const char *expression; bool selected; };
  const Case cases[] = {
      {"1 + 2 * 3 == 7", true},
      {"defined(X) && !defined(MISSING)", true},
      {"defined X && MISSING == 0", true},
      {"(8 >> 1) == 4 && (1 << 3) == 8", true},
      {"(5 & 3) == 1 && (5 | 2) == 7 && (5 ^ 1) == 4", true},
      {"8 / 2 == 4 && 8 % 3 == 2", true},
      {"-3 < -2 && ~0 == -1", true},
      {"'a' == 97", true},
      {"0 && (1 / 0)", false},
      {"1 || (1 / 0)", true},
      {"0 ? 1 / 0 : 3", true},
      {"1 ? 3 : 1 / 0", true},
      {"-1 < 1U", false},
      {"0xffffffffffffffffULL == -1", true},
      {"(0 ? 1U : -1) > 0", true},
      {"1 ? 0 : 1 ? 1 : 0", false},
      {"0 && (1 << 64)", false},
      {"MISSING", false},
      {"1 != 2 && 3 >= 3 && 2 <= 3", true},
      {"077 == 63 && 0x10 == 16 && 16UL == 16", true},
  };
  for (const auto &test : cases) {
    const std::string source = std::string("#define X 1\n#if ") + test.expression +
        "\nint yes;\n#else\nint no;\n#endif\n";
    expect(source, test.selected ? "int yes ;" : "int no ;");
  }
  expect("#if 0\n@ 0x 123bad\n#error ignored\n#define HIDDEN 1\n#include \"missing.h\"\n"
         "#if invalid expression\n@\n#endif\n#elif 0\nint no;\n#elif 1\nint yes;\n"
         "#else\nint no;\n#endif\n#ifdef HIDDEN\nint no;\n#endif\n", "int yes ;");
  expect("#ifndef MISSING\n#define PRESENT\n#endif\n#ifdef PRESENT\nint yes;\n#endif\n",
         "int yes ;");
  expect("#if 1\nint yes;\n#elif 1 / 0\nint no;\n#endif\n", "int yes ;");
}

void normalization() {
  expect("#defi\\\nne VALUE 1 \\\n+ 2\nint value = VALUE;\n", "int value = 1 + 2 ;");
  expect("#define VALUE /* multiple\nlines */ 3\nVALUE\n", "3");
  expect("int va\\\nlue; // continued comment \\\n@ ignored\n", "int value ;");
  expect("#define VALUE 3\r\n#if VALUE\rint value;\r\n#endif", "int value ;");
  expect("%:define VALUE 3\nVALUE\n", "3");
  Context ctx("#define VALUE 8\n\nint value;\nVALUE\n");
  const auto keyword = ctx.pp.next();
  check(keyword.loc.line == 3 && keyword.loc.col == 1, "directive lines shifted locations");
  ctx.pp.next();
  ctx.pp.next();
  const auto value = ctx.pp.next();
  check(value.int_val == 8 && value.loc.line == 4 && value.loc.col == 1,
        "expanded token has the wrong invocation location");
  Context spliced("int value =\n \\\n 8;\n");
  spliced.pp.next(); spliced.pp.next(); spliced.pp.next();
  const auto literal = spliced.pp.next();
  check(literal.loc.line == 3 && literal.loc.col == 2, "line splice lost the physical location");
}

void error(const std::string &source, const std::string &message) {
  Context ctx(source);
  ctx.pp.run();
  ctx.spelling();
  check(ctx.diag.errorCount() == 1, "expected one diagnostic:\n" + ctx.output.str());
  check(ctx.diag.diagnostics()[0].message == message, "wrong diagnostic:\n" + ctx.output.str());
  check(ctx.output.str().find('^') != std::string::npos, "missing source caret");
  check(ctx.pp.next().isEof(), "errors must end the stream for parser recovery");
}

void diagnostics() {
  error("#include \"missing-preprocessor-test.h\"\n", "include file 'missing-preprocessor-test.h' not found");
  error("#if 1\nint value;\n", "unterminated conditional directive");
  error("#else\n", "#else without matching #if");
  error("#if 0\n#else\n#else\n#endif\n", "duplicate #else");
  error("#if 0\n#else\n#elif 1\n#endif\n", "#elif after #else");
  error("#if defined()\n#endif\n", "expected macro name after defined");
  error("#if 1 / 0\n#endif\n", "division by zero in #if expression");
  error("#if 1 << 64\n#endif\n", "invalid shift count in #if expression");
  error("#if 1.0\n#endif\n", "expected integer constant in #if expression");
  error("#if 18446744073709551616U\n#endif\n", "integer constant is out of range in #if expression");
  error("#define F(x) x\n", "function-like macros are not supported yet");
  error("#define X a ## b\n", "macro '#' and '##' operators are not supported yet");
  error("#define X 1\n#define X 2\n", "incompatible redefinition of macro 'X'");
  error("#undef\n", "expected one macro name after #undef");
  error("#error stop now\n", "#error stop now");
  error("#unknown\n", "unsupported preprocessing directive '#unknown'");
  error("0xE+12", "invalid preprocessing token '0xE+12'");
  Context spliced("\"a\\\n\\q\"\n");
  spliced.spelling();
  check(spliced.diag.errorCount() == 1 && spliced.diag.diagnostics()[0].line == 2 &&
        spliced.diag.diagnostics()[0].col == 1, "escape diagnostic lost its physical location");
  Context arithmetic("#if 1 \\\n/ 0\n#endif\n");
  arithmetic.pp.run();
  check(arithmetic.diag.errorCount() == 1 && arithmetic.diag.diagnostics()[0].line == 2 &&
        arithmetic.diag.diagnostics()[0].col == 1, "arithmetic diagnostic lost its physical location");
}

void headers() {
  const std::string root = std::string(CC1_TEST_SOURCE_DIR) + "/preprocessor/";
  Context ctx("#define HEADER \"include/values.h\"\n#include HEADER\n"
              "#include \"include/values.h\"\n#include \"include/once.h\"\n"
              "#include \"include/./once.h\"\nint main(void) { return BASE + MORE; }\n",
              root + "test.c");
  check(ctx.spelling() == "int header_value ; int nested_value ; int once_value ; "
                         "int main ( void ) { return 6 + 1 ; }", ctx.output.str());
  check(!ctx.diag.hasErrors(), ctx.output.str());
  Context angled("#define library missing\n#include <library.h>\nLIB_VALUE\n",
                 root + "test.c", {root + "search"});
  check(angled.spelling() == "int library_value ; 9", angled.output.str());
  Context headerError("#include \"include/invalid_lexical.h\"\n", root + "test.c");
  headerError.spelling();
  check(headerError.diag.errorCount() == 1, headerError.output.str());
  const auto &record = headerError.diag.diagnostics()[0];
  check(record.filename == root + "include/invalid_lexical.h" && record.line == 1 && record.col == 5,
        "header diagnostic has the wrong file or position");
  check(headerError.output.str().find("    1 | int @;\n      |     ^\n") != std::string::npos,
        "header source excerpt is missing");
  Context guarded("#include \"include/recursive.h\"\n", root + "test.c");
  guarded.pp.run();
  check(guarded.diag.errorCount() == 1 && guarded.diag.diagnostics()[0].message ==
        "include nesting limit exceeded", "unguarded inclusion did not stop");
  Context unbalanced("#if 1\n#include \"include/unbalanced.h\"\n#endif\n", root + "test.c");
  unbalanced.pp.run();
  check(unbalanced.diag.errorCount() == 1 && unbalanced.diag.diagnostics()[0].message ==
        "#endif without matching #if", "conditional groups crossed an include boundary");
}
} // namespace

int main() {
  try {
    macros(); conditions(); normalization(); diagnostics(); headers();
  } catch (const std::exception &failure) {
    std::cerr << failure.what() << '\n';
    return 1;
  }
  return 0;
}
