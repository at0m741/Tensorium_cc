#pragma once
#include "cc1/Parse/Parser.hpp"

class ParserTestHelper {
public:
  explicit ParserTestHelper(Parser &p) : parser(p) {}

  Type *parseTypeSpec() { return parser.parseTypeSpec(); }
  Type *parseDeclarator(Type *base, std::string &nameOut) {
    return parser.parseDeclarator(base, nameOut);
  }
  Type *parsePointer(Type *base) { return parser.parsePointer(base); }
  Expr *parseExpr() { return parser.parseExpr(); }

  TokenKind currentKind() const { return parser._cur.kind; }

private:
  Parser &parser;
};
