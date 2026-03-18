#include "lexer/Lexer.hpp"
#include "lexer/Token.hpp"
#include <fstream>
#include <iostream>
#include <sstream>

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "usage: cc1 <file.c>\n");
    return 1;
  }

  std::ifstream file(argv[1]);
  if (!file) {
    fprintf(stderr, "cc1: error: cannot open '%s'\n", argv[1]);
    return 1;
  }
  std::ostringstream ss;
  ss << file.rdbuf();
  std::string src = ss.str();

  Lexer lexer(src, argv[1]);
  for (auto &tok : lexer.tokenizeAll()) {
    printf("%s:%u:%u\t%s\t'%s'\n", tok.loc.filename, tok.loc.line, tok.loc.col,
           tokenKindName(tok.kind), tok.text.c_str());
  }
  return 0;
}
