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

void functionMacros() {
  expect("#define ADD(x,y) ((x) + (y))\nADD(2,3)\n", "( ( 2 ) + ( 3 ) )");
  expect("#define F(x) x + x\nF(F(1))\n", "1 + 1 + 1 + 1");
  expect("#define F(x) x\n#define ALIAS F\nALIAS (7)\nF(F)(8)\n", "7 F ( 8 )");
  expect("#define ID(x) x\n#define F(x) x+1\nID(F)(4)\n", "4 + 1");
  expect("#define F(x) G(x)\n#define G(x) F(x)\nF(1) G(2)\n", "F ( 1 ) G ( 2 )");
  expect("#define F(x) x\nF((1,2))\nF\n(\n3\n)\n", "( 1 , 2 ) 3");
  expect("#define ZERO() 0\n#define EMPTY(x) x\nZERO() EMPTY() EMPTY( )\n", "0");
  expect("#define IGNORE(x) 3\n#define BAD(x) x\nIGNORE(BAD(1,2))\n", "3");
  expect("#define F (x)\nF(3)\n", "( x ) ( 3 )");
  expect("#define F(x) x\n#define F(x) /* same */ x\nF(3)\n", "3");
  expect("#define F(x) x\nF\n#define FOO 3\nFOO\n", "F 3");
  expect("#define F(x) x\n#if defined(F) && F(1)\nint yes;\n#endif\n", "int yes ;");
  expect("#define f(x) x x\n#define h(x) #x\nh(f(1))\n", "f(1)");
  expect("#define f(x) x x\n#define g(x) h(x)\n#define h(x) #x\ng(f(1))\n", "1 1");
  // C99 6.10.3.5 example: rescan, aliases, commas and indirect recursion.
  expect("#define x 3\n#define f(a) f(x * (a))\n#undef x\n#define x 2\n"
         "#define g f\n#define z z[0]\n#define h g(~\n#define m(a) a(w)\n"
         "#define w 0,1\n#define t(a) a\n"
         "f(y+1) + f(f(z)) % t(t(g)(0) + t)(1);\n"
         "g(x+(3,4)-w) | h 5) & m(f)^m(m);\n",
         "f ( 2 * ( y + 1 ) ) + f ( 2 * ( f ( 2 * ( z [ 0 ] ) ) ) ) % "
         "f ( 2 * ( 0 ) ) + t ( 1 ) ; f ( 2 * ( 2 + ( 3 , 4 ) - 0 , 1 ) ) | "
         "f ( 2 * ( ~ 5 ) ) & f ( 2 * ( 0 , 1 ) ) ^ m ( 0 , 1 ) ;");
}

void stringificationAndPasting() {
  expect("#define N 4\n#define STR(x) #x\n#define XSTR(x) STR(x)\nSTR(N); XSTR(N)\n", "N ; 4");
  expect("#define STR(x) #x\nSTR(  a+\n  b /* comment */ c  )\n", "a+ b c");
  Context strings("#define STR(x) #x\nSTR(\"a\\n\" '\\t'); STR()\n");
  check(strings.pp.next().str_val == "\"a\\n\" '\\t'", "stringification lost escapes");
  strings.pp.next(); // separator
  check(strings.pp.next().str_val.empty(), "empty stringification changed");
  check(!strings.diag.hasErrors(), strings.output.str());
  expect("#define STR(x) #x\nSTR(%:) STR(<:x:>)\n", "%:<:x:>");
  Context adjacent("#define STR(x) #x\n\"prefix: \" STR(value) \"\\0suffix\";\n");
  const auto joined = adjacent.pp.next();
  check(joined.str_val == std::string("prefix: value\0suffix", 20), "adjacent strings lost bytes");
  check(adjacent.pp.next().kind == TokenKind::SEMICOLON, "adjacent strings were not joined");
  Context escapeBoundary("\"\\\\\" \"n\"\n");
  check(escapeBoundary.pp.next().str_val == "\\n",
        "adjacent strings changed an escape at the boundary");
  expect("#define CAT(a,b) a ## b\n#define XY 9\nCAT(X,Y)\n", "9");
  expect("#define CAT(a,b) a ## b\nCAT(,x) CAT(x,) CAT(,)\n", "x x");
  expect("#define CAT(a,b) a ## b\nCAT(a b,c d)\n", "a bc d");
  expect("#define CAT(a,b,c) a ## b ## c\nCAT(a,,b) CAT(,,) CAT(,x,)\n", "ab x");
  expect("#define CAT(a,b) a ## b\nCAT(+,=) CAT(1,.5) CAT(1,e3)\n", "+= 1.5 1e3");
  expect("#define OBJECT pre ## fix\nOBJECT\n", "prefix");
  expect("#define A 1\n#define CAT(a,b) a ## b\n#define XCAT(a,b) CAT(a,b)\n"
         "CAT(A,2) XCAT(A,2)\n", "A2 12");
  expect("#define CAT(a,b) a ## b\n#define CALL(x) x+1\nCAT(CA,LL)(2)\n", "2 + 1");
  expect("#define HASH #\n#define CAT(a,b) a ## b\n#define STR(x) #x\n"
         "#define XSTR(x) STR(x)\nXSTR(CAT(HASH,HASH))\n", "HASHHASH");
  // A generated ## token must not become another paste operator.
  expect("#define HASH_HASH # ## #\n#define STR(x) #x\n#define XSTR(x) STR(x)\n"
         "XSTR(HASH_HASH)\n", "##");
}

void variadicMacros() {
  expect("#define ARGS(...) __VA_ARGS__\nARGS(1,2,3) ARGS()\n", "1 , 2 , 3");
  expect("#define CALL(f,...) f(__VA_ARGS__)\nCALL(fun,1,(2,3)) CALL(fun,)\n",
         "fun ( 1 , ( 2 , 3 ) ) fun ( )");
  expect("#define STR(...) #__VA_ARGS__\nSTR(a, b,c)\n", "a, b,c");
  expect("#define CAT(prefix,...) prefix ## __VA_ARGS__\nCAT(pre,fix) CAT(pre,)\n",
         "prefix pre");
  expect("#define VALUE 3\n#define ARGS(...) __VA_ARGS__\nARGS(VALUE,ARGS(4))\n", "3 , 4");
}

void predefinedMacros() {
  expect("__STDC__ __STDC_VERSION__ __STDC_HOSTED__\n", "1 199901L 0");
  expect("#if defined(__FILE__) && defined __LINE__ && __STDC_VERSION__ == 199901L\n"
         "__LINE__\n#endif\n#ifndef __cplusplus\nint c;\n#endif\n", "2 int c ;");
  expect("#define LINE __LINE__\n#define F(x) __LINE__ + x\nLINE\nF(\n__LINE__\n)\n", "3 6 + 5");
  Context file("__FILE__\n", "path/with\"quote\\slash\n\r\t.c");
  check(file.pp.next().str_val == file.file.filename, "__FILE__ was not escaped");
  Context clock("__DATE__; __TIME__; __DATE__; __TIME__\n");
  const auto date = clock.pp.next().str_val;
  clock.pp.next();
  const auto time = clock.pp.next().str_val;
  clock.pp.next();
  check(date.size() == 11 && date[3] == ' ' && date[6] == ' ', "invalid __DATE__");
  check(time.size() == 8 && time[2] == ':' && time[5] == ':', "invalid __TIME__");
  check(clock.pp.next().str_val == date, "date changed during preprocessing");
  clock.pp.next();
  check(clock.pp.next().str_val == time,
        "date and time changed during preprocessing");
  for (auto standard : {LangOptions::C89, LangOptions::C11, LangOptions::C17}) {
    std::ostringstream output;
    const std::string source = "#ifdef __STDC_VERSION__\n__STDC_VERSION__\n#else\n0\n#endif\n";
    DiagnosticEngine diag("standard.c", source, output, DiagnosticColor::Never);
    SourceManager sources(diag);
    LangOptions options;
    options.std = standard;
    Preprocessor pp(sources, sources.add("standard.c", source), diag, options);
    check(pp.next().text == (standard == LangOptions::C89 ? "0" :
          standard == LangOptions::C11 ? "201112L" : "201710L"), "wrong standard macro");
  }
  std::ostringstream output;
  const std::string source = "#define F(...) __VA_ARGS__\n";
  DiagnosticEngine diag("c89.c", source, output, DiagnosticColor::Never);
  SourceManager sources(diag);
  Preprocessor c89(sources, sources.add("c89.c", source), diag, LangOptions::forC89());
  check(!c89.run() && diag.errorCount() == 1 && diag.diagnostics()[0].message ==
        "variadic macros require C99 or later", "C89 accepted a variadic macro");
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
  error("#define F(x,x) x\n", "duplicate macro parameter 'x'");
  error("#define F(x,) x\n", "expected macro parameter name");
  error("#define F(x x) x\n", "expected ')' after macro parameters");
  error("#define F( x\n", "expected ')' after macro parameters");
  error("#define F(1) x\n", "expected macro parameter name");
  error("#define F(x) x\nF(1,2)\n", "wrong number of arguments to macro 'F'");
  error("#define F() 1\nF(1)\n", "wrong number of arguments to macro 'F'");
  error("#define F(x,y) x\nF()\n", "wrong number of arguments to macro 'F'");
  error("#define F(x) x\nF(1\n", "unterminated invocation of macro 'F'");
  error("#define F(x) #y\n", "'#' must be followed by a macro parameter");
  error("#define F(x) #\n", "'#' must be followed by a macro parameter");
  error("#define X ## a\n", "'##' requires a token on each side");
  error("#define X a ##\n", "'##' requires a token on each side");
  error("#define X a ## ## b\n", "'##' requires a token on each side");
  error("#define CAT(a,b) a ## b\nCAT(x,+)\n",
        "token paste does not form one preprocessing token: 'x+'");
  error("#define CAT(a,b) a ## b\nCAT(/,*)\n",
        "token paste does not form one preprocessing token: '/*'");
  error("#define CAT(a,b) a ## b\nCAT(/,/)\n",
        "token paste does not form one preprocessing token: '//'");
  error("#define F(x) x\n#define F(y) y\n", "incompatible redefinition of macro 'F'");
  error("#define F(x) x\n#define F x\n", "incompatible redefinition of macro 'F'");
  error("#define F(x,...) x\nF(1)\n", "wrong number of arguments to macro 'F'");
  error("#define F(...,x) x\n", "expected ')' after macro parameters");
  error("#define X __VA_ARGS__\n", "__VA_ARGS__ is only valid in a variadic macro");
  error("#define __LINE__ 3\n", "cannot redefine predefined macro '__LINE__'");
  error("#undef __FILE__\n", "cannot undefine predefined macro '__FILE__'");
  error("#define A a,b\n#define F(x) G(x)\n#define G(x) x\nF(A)\n",
        "wrong number of arguments to macro 'G'");
  std::string nested = "#define ID(x) x\n";
  for (int i = 0; i < 129; ++i) nested += "ID(";
  nested += '1';
  nested.append(129, ')');
  error(nested, "macro expansion limit exceeded");
  std::string chain;
  for (int i = 0; i < 130; ++i)
    chain += "#define A" + std::to_string(i) + " A" + std::to_string(i + 1) + "\n";
  error(chain + "A0\n", "macro expansion limit exceeded");
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
  Context location("#include \"include/location.h\"\n__FILE__ __LINE__ HEADER_LINE\n",
                   root + "test.c");
  check(location.pp.next().str_val == root + "include/location.h", "wrong header __FILE__");
  check(location.pp.next().int_val == 1, "wrong header __LINE__");
  check(location.pp.next().str_val == root + "test.c", "__FILE__ did not return to the parent");
  check(location.pp.next().int_val == 2 && location.pp.next().int_val == 2,
        "__LINE__ did not return to the parent invocation");
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
    macros(); functionMacros(); stringificationAndPasting(); variadicMacros();
    predefinedMacros(); conditions(); normalization(); diagnostics(); headers();
  } catch (const std::exception &failure) {
    std::cerr << failure.what() << '\n';
    return 1;
  }
  return 0;
}
