#include "cc1/Diagnostic.hpp"
#include "lexer/Lexer.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {
void check(bool condition, const std::string &message) {
  if (!condition)
    throw std::runtime_error(message);
}

void checkLexError(const std::string &source, const std::string &message,
                   uint32_t line, uint32_t col, const std::string &excerpt) {
  std::ostringstream output;
  DiagnosticEngine diagnostics("test.c", source, output, DiagnosticColor::Never);
  Lexer lexer(source, "test.c", diagnostics);
  const auto tokens = lexer.tokenizeAll();
  check(!tokens.empty() && tokens.back().isError(), "missing ERROR token");
  check(tokens.back().loc.line == line && tokens.back().loc.col == col,
        "incorrect ERROR token location");
  check(diagnostics.hasErrors() && diagnostics.errorCount() == 1,
        "error was not registered exactly once");
  const auto &record = diagnostics.diagnostics().front();
  check(record.message == message && record.line == line && record.col == col,
        "incorrect stored diagnostic");
  const std::string header = "test.c:" + std::to_string(line) + ":" +
                             std::to_string(col) + ": error: " + message + "\n";
  check(output.str() == header + excerpt, "incorrect diagnostic:\n" + output.str());
}

void testLocations() {
  checkLexError("int x = @;\n", "unknown character", 1, 9,
                "    1 | int x = @;\n      |         ^\n");
  checkLexError("int x;\n/* comment\nmore", "unterminated block comment", 2, 1,
                "    2 | /* comment\n      | ^~\n");
  checkLexError("/*", "unterminated block comment", 1, 1,
                "    1 | /*\n      | ^~\n");
  checkLexError("/*x", "unterminated block comment", 1, 1,
                "    1 | /*x\n      | ^~\n");
  checkLexError("\"hello\n", "unterminated string literal", 1, 1,
                "    1 | \"hello\n      | ^\n");
  checkLexError("'x", "unterminated character literal", 1, 1,
                "    1 | 'x\n      | ^\n");
  checkLexError("\"a\\q\"", "invalid escape sequence", 1, 3,
                "    1 | \"a\\q\"\n      |   ^~\n");
  checkLexError("'\\q'", "invalid escape sequence", 1, 2,
                "    1 | '\\q'\n      |  ^~\n");
  checkLexError("\t@\r\n", "unknown character", 1, 2,
                "    1 |         @\n      |         ^\n");
}

void testNumbers() {
  checkLexError("0x", "invalid numeric literal", 1, 1,
                "    1 | 0x\n      | ^~\n");
  checkLexError("1e+", "invalid numeric literal", 1, 1,
                "    1 | 1e+\n      | ^~~\n");
  checkLexError("999999999999999999999999999999", "numeric literal is out of range",
                1, 1, "    1 | 999999999999999999999999999999\n"
                      "      | ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
  checkLexError("1e9999", "numeric literal is out of range", 1, 1,
                "    1 | 1e9999\n      | ^~~~~~\n");

  std::ostringstream output;
  const std::string source = "/* ok */ .5 1. 1.5 . ...";
  DiagnosticEngine diagnostics("test.c", source, output);
  Lexer lexer(source, "test.c", diagnostics);
  const auto tokens = lexer.tokenizeAll();
  check(tokens.size() == 6 && tokens[0].kind == TokenKind::FLOAT_LIT &&
        tokens[0].float_val == 0.5 && tokens[1].kind == TokenKind::FLOAT_LIT &&
        tokens[1].float_val == 1.0 && tokens[2].kind == TokenKind::FLOAT_LIT &&
        tokens[2].float_val == 1.5 && tokens[3].kind == TokenKind::DOT &&
        tokens[4].kind == TokenKind::ELLIPSIS && tokens[5].isEof(),
        "valid floats or punctuation changed");
  check(!diagnostics.hasErrors() && output.str().empty(),
        "valid input produced diagnostics");
}

void testLevelsAndColor() {
  std::ostringstream plain;
  DiagnosticEngine diagnostics("test.c", "x", plain);
  diagnostics.warning({"test.c", 1, 1}, "a warning");
  diagnostics.note({"test.c", 1, 1}, "a note");
  check(!diagnostics.hasErrors(), "warning or note counted as an error");
  diagnostics.error({"test.c", 1, 1}, "an error");
  check(diagnostics.errorCount() == 1 && diagnostics.diagnostics().size() == 3,
        "incorrect diagnostic counts");
  diagnostics.fatal({}, "a fatal error");
  check(diagnostics.errorCount() == 2 &&
        plain.str().find("fatal error: a fatal error") != std::string::npos,
        "fatal diagnostic was not counted or rendered");
  check(plain.str().find("warning: a warning") != std::string::npos &&
        plain.str().find("note: a note") != std::string::npos &&
        plain.str().find("\033[") == std::string::npos,
        "auto color should be disabled for an injected stream");

  std::ostringstream colored;
  DiagnosticEngine forced("test.c", "@", colored, DiagnosticColor::Always);
  forced.error({"test.c", 1, 1}, "unknown character");
  check(colored.str().find("\033[1;31merror: ") != std::string::npos &&
        colored.str().find("\033[1;32m^\033[0m") != std::string::npos,
        "forced error or caret color is missing");
}

void testRenderingBounds() {
  std::ostringstream output;
  DiagnosticEngine diagnostics("test.c", "x\n", output, DiagnosticColor::Never);
  diagnostics.error({"test.c", 1, 1}, "long range", 100);
  diagnostics.error({"test.c", 2, 1}, "at end of file");
  diagnostics.error({}, "without a source location");
  diagnostics.error({"other.c", 1, 1}, "another file");
  check(output.str() ==
        "test.c:1:1: error: long range\n    1 | x\n      | ^\n"
        "test.c:2:1: error: at end of file\n    2 | \n      | ^\n"
        "test.c: error: without a source location\n"
        "other.c:1:1: error: another file\n",
        "range clipping or missing source handling failed");
}

void testPeek() {
  std::ostringstream output;
  DiagnosticEngine diagnostics("test.c", "@", output);
  Lexer lexer("@", "test.c", diagnostics);
  check(lexer.peek().isError() && lexer.peek().isError() && lexer.next().isError(),
        "peek did not preserve the error token");
  check(diagnostics.errorCount() == 1 && lexer.next().isEof(),
        "peek emitted a duplicate diagnostic");
}
} // namespace

int main() {
  try {
    testLocations();
    testNumbers();
    testLevelsAndColor();
    testRenderingBounds();
    testPeek();
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
  return 0;
}
