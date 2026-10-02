#include "lexer/Lexer.hpp"
#include "lexer/Token.hpp"
#include "cc1/Diagnostic.hpp"
#include <fstream>
#include <iostream>
#include <sstream>

int main(int argc, char **argv) {
  DiagnosticColor color = DiagnosticColor::Auto;
  const char *filename = nullptr;
  bool options = true;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (options && arg == "--") {
      options = false;
    } else if (options && arg == "-fcolor-diagnostics") {
      color = DiagnosticColor::Always;
    } else if (options && arg == "-fno-color-diagnostics") {
      color = DiagnosticColor::Never;
    } else if ((options && !arg.empty() && arg[0] == '-') || filename) {
      DiagnosticEngine diagnostics("cc1", "", std::cerr, color);
      diagnostics.error({}, filename ? "expected a single input file"
                                    : "unknown option '" + arg + "'");
      return 1;
    } else {
      filename = argv[i];
    }
  }
  if (!filename) {
    DiagnosticEngine diagnostics("cc1", "", std::cerr, color);
    diagnostics.error({}, "no input file");
    std::cerr << "usage: cc1 [-fcolor-diagnostics|-fno-color-diagnostics] <file.c>\n";
    return 1;
  }

  std::ifstream file(filename);
  if (!file) {
    DiagnosticEngine diagnostics("cc1", "", std::cerr, color);
    diagnostics.error({}, "cannot open '" + std::string(filename) + "'");
    return 1;
  }
  std::ostringstream ss;
  ss << file.rdbuf();
  std::string src = ss.str();

  DiagnosticEngine diagnostics(filename, src, std::cerr, color);
  Lexer lexer(src, filename, diagnostics);
  const auto tokens = lexer.tokenizeAll();
  if (diagnostics.hasErrors())
    return 1;

  for (const auto &tok : tokens) {
    printf("%s:%u:%u\t%s\t'%s'\n", tok.loc.filename, tok.loc.line, tok.loc.col,
           tokenKindName(tok.kind), tok.text.c_str());
  }
  return 0;
}
