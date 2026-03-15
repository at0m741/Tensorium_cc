#include "Lexer.hpp"
#include "Token.hpp"
#include <unordered_map>

const std::unordered_map<std::string, TokenKind> Lexer::_keywords = {
    {"auto", TokenKind::KW_AUTO},         {"break", TokenKind::KW_BREAK},
    {"case", TokenKind::KW_CASE},         {"char", TokenKind::KW_CHAR},
    {"const", TokenKind::KW_CONST},       {"continue", TokenKind::KW_CONTINUE},
    {"default", TokenKind::KW_DEFAULT},   {"do", TokenKind::KW_DO},
    {"double", TokenKind::KW_DOUBLE},     {"else", TokenKind::KW_ELSE},
    {"enum", TokenKind::KW_ENUM},         {"extern", TokenKind::KW_EXTERN},
    {"float", TokenKind::KW_FLOAT},       {"for", TokenKind::KW_FOR},
    {"goto", TokenKind::KW_GOTO},         {"if", TokenKind::KW_IF},
    {"int", TokenKind::KW_INT},           {"long", TokenKind::KW_LONG},
    {"register", TokenKind::KW_REGISTER}, {"return", TokenKind::KW_RETURN},
    {"short", TokenKind::KW_SHORT},       {"signed", TokenKind::KW_SIGNED},
    {"sizeof", TokenKind::KW_SIZEOF},     {"static", TokenKind::KW_STATIC},
    {"struct", TokenKind::KW_STRUCT},     {"switch", TokenKind::KW_SWITCH},
    {"typedef", TokenKind::KW_TYPEDEF},   {"union", TokenKind::KW_UNION},
    {"unsigned", TokenKind::KW_UNSIGNED}, {"void", TokenKind::KW_VOID},
    {"volatile", TokenKind::KW_VOLATILE}, {"while", TokenKind::KW_WHILE},
};

char Lexer::cur() const { return _pos < _src.size() ? _src[_pos] : '\0'; }

char Lexer::look(int offset) const {
  size_t p = _pos + offset;
  return p < _src.size() ? _src[p] : '\0';
}

char Lexer::advance() {
  char c = cur();
  ++_pos;

  if (c == '\n') {
    ++_line;
    _col = 1;
  } else {
    ++_col;
  }

  return c;
}

SourceLoc Lexer::loc() const { return {_filename, _line, _col}; }

Token Lexer::make(TokenKind k, std::string text) {
  return Token(k, std::move(text), loc());
}

Token Lexer::Error(const std::string &msg) {
  fprintf(stderr, "%s:%u:%u: error: %s\n", _filename, _line, _col, msg.c_str());
  return make(TokenKind::ERROR, "");
}
