#pragma once
#include "Token.hpp"
#include <cstdint>
#include <string>
#include <sys/types.h>
#include <unordered_map>
#include <vector>

class Lexer {
public:
  Lexer(const std::string &source, const char *filename);

  Token next(); /* Consume and return next token */
  Token peek(); /* lookup, but don't consume (il est punis miskine) */
	
  /* Just for debuging and Tests (the fun part) */
  std::vector<Token> tokenizeAll();

private:
  std::string _src;
  const char *_filename;
  size_t _pos = 0;
  uint32_t _col = 1;
  uint32_t _line = 1;

  bool _hasPeek = false;
  Token _peekTok;

  static const std::unordered_map<std::string, TokenKind> _keywords;

  char cur() const;                /* check the current char */
  char look(int offset = 1) const; /* lookahead */
  char advance();                  /* char consumer */
  SourceLoc loc() const;

  void skipWhiteSpaceAndComments();
  Token lexIdent();
  Token lexNumber();
  Token lexChar();
  Token lexString();
  Token lexPunct();

  Token make(TokenKind k, std::string text);
  Token Error(const std::string &msg);
};
