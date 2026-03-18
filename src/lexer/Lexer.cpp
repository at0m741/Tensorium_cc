#include "Lexer.hpp"
#include "Token.hpp"
#include <cctype>
#include <string>
#include <unordered_map>

Lexer::Lexer(const std::string &source, const char *filename)
    : _src(source), _filename(filename) {}

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

void Lexer::skipWhiteSpaceAndComments() {
  while (_pos < _src.size()) {
    if (std::isspace(cur())) {
      advance();
    } else if (cur() == '/' && look() == '*') {
      advance();
      advance();
      bool closed = false;
      while (_pos + 1 < _src.size()) {
        if (cur() == '*' && look() == '/') {
          advance();
          advance();
          closed = true;
          break;
        }
        advance();
      }
      if (!closed)
        Error("Unterminated block comment");
    } else
      break;
  }
}

Token Lexer::lexIdent() {
  SourceLoc tmp = loc();
  std::string s;

  while (std::isalnum(cur()) || cur() == '_') {
    s += advance();
  }
  auto it = _keywords.find(s);
  TokenKind k = (it != _keywords.end()) ? it->second : TokenKind::IDENT;
  return Token(k, s, tmp);
}

Token Lexer::lexNumber() {
  SourceLoc tmp = loc();
  std::string s;
  bool isFloat = false;

  if (cur() == '0' && (look() == 'x' || look() == 'X')) {
    s += advance();
    s += advance();
    while (std::isxdigit(cur()))
      s += advance();
  } else {
    while (std::isdigit(cur()))
      s += advance();
    if (cur() == '.' && std::isdigit(look())) {
      isFloat = true;
      s += advance();
      while (std::isdigit(cur()))
        s += advance();
    }
    if (cur() == 'e' || cur() == 'E') {
      isFloat = true;
      s += advance();
      if (cur() == '+' || cur() == '-')
        s += advance();
      while (std::isdigit(cur()))
        s += advance();
    }
  }

  while (cur() == 'u' || cur() == 'U' || cur() == 'l' || cur() == 'L' ||
         cur() == 'f' || cur() == 'F')
    s += advance();

  Token tok(isFloat ? TokenKind::FLOAT_LIT : TokenKind::INT_LIT, s, tmp);
  if (isFloat)
    tok.float_val = std::stod(s);
  else
    tok.int_val = std::stoll(s, nullptr, 0);
  return tok;
}

Token Lexer::lexChar() {
  SourceLoc tmp = loc();
  std::string s;
  char val = 0;

  advance();

  if (cur() == '\0' || cur() == '\n')
    return Error("Unterminated character literal");

  if (cur() == '\\') {
    s += advance(); // '\'
    switch (cur()) {
    case 'n':
      val = '\n';
      s += advance();
      break;
    case 't':
      val = '\t';
      s += advance();
      break;
    case 'r':
      val = '\r';
      s += advance();
      break;
    case '0':
      val = '\0';
      s += advance();
      break;
    case '\\':
      val = '\\';
      s += advance();
      break;
    case '\'':
      val = '\'';
      s += advance();
      break;
    case '"':
      val = '"';
      s += advance();
      break;
    default:
      return Error("Invalid escape sequence");
    }
  } else {
    val = cur();
    s += advance();
  }

  if (cur() != '\'')
    return Error("Unterminated character literal");
  advance();

  Token tok(TokenKind::CHAR_LIT, s, tmp);
  tok.char_val = val;
  return tok;
}

Token Lexer::lexString() {
  SourceLoc tmp = loc();
  std::string s;
  advance();

  while (_pos < _src.size() && cur() != '"') {
    if (cur() == '\0' || cur() == '\n')
      return Error("Unterminated string literal");
    if (cur() == '\\') {
      s += advance();
      switch (cur()) {
      case 'n':
        s += '\n';
        advance();
        break;
      case 't':
        s += '\t';
        advance();
        break;
      case 'r':
        s += '\r';
        advance();
        break;
      case '0':
        s += '\0';
        advance();
        break;
      case '\\':
        s += '\\';
        advance();
        break;
      case '"':
        s += '"';
        advance();
        break;
      case '\'':
        s += '\'';
        advance();
        break;
      default:
        return Error("Invalid escape sequence");
      }
    } else {
      s += advance();
    }
  }

  if (cur() != '"')
    return Error("Unterminated string literal");
  advance();

  Token tok(TokenKind::STRING_LIT, s, tmp);
  tok.str_val = s;
  return tok;
}

Token Lexer::lexPunct() {
  SourceLoc tmp = loc();
  char c = cur();
  char n = look();
  auto makePunct = [&](TokenKind kind, const char *text) {
    return Token(kind, text, tmp);
  };

  switch (c) {
  case '(':
    advance();
    return makePunct(TokenKind::L_PAREN, "(");
  case ')':
    advance();
    return makePunct(TokenKind::R_PAREN, ")");
  case '{':
    advance();
    return makePunct(TokenKind::L_BRACE, "{");
  case '}':
    advance();
    return makePunct(TokenKind::R_BRACE, "}");
  case '[':
    advance();
    return makePunct(TokenKind::L_BRACKET, "[");
  case ']':
    advance();
    return makePunct(TokenKind::R_BRACKET, "]");
  case ';':
    advance();
    return makePunct(TokenKind::SEMICOLON, ";");
  case ':':
    advance();
    return makePunct(TokenKind::COLON, ":");
  case ',':
    advance();
    return makePunct(TokenKind::COMMA, ",");
  case '?':
    advance();
    return makePunct(TokenKind::QUESTION, "?");
  case '~':
    advance();
    return makePunct(TokenKind::TILDE, "~");

  case '.':
    if (n == '.' && look(2) == '.') {
      advance();
      advance();
      advance();
      return makePunct(TokenKind::ELLIPSIS, "...");
    }
    advance();
    return makePunct(TokenKind::DOT, ".");

  case '#':
    if (n == '#') {
      advance();
      advance();
      return makePunct(TokenKind::HASHHASH, "##");
    }
    advance();
    return makePunct(TokenKind::HASH, "#");

  case '+':
    if (n == '+') {
      advance();
      advance();
      return makePunct(TokenKind::PLUSPLUS, "++");
    }
    if (n == '=') {
      advance();
      advance();
      return makePunct(TokenKind::PLUS_ASSIGN, "+=");
    }
    advance();
    return makePunct(TokenKind::PLUS, "+");

  case '-':
    if (n == '-') {
      advance();
      advance();
      return makePunct(TokenKind::MINUSMINUS, "--");
    }
    if (n == '=') {
      advance();
      advance();
      return makePunct(TokenKind::MINUS_ASSIGN, "-=");
    }
    if (n == '>') {
      advance();
      advance();
      return makePunct(TokenKind::ARROW, "->");
    }
    advance();
    return makePunct(TokenKind::MINUS, "-");

  case '*':
    if (n == '=') {
      advance();
      advance();
      return makePunct(TokenKind::MUL_ASSIGN, "*=");
    }
    advance();
    return makePunct(TokenKind::MUL, "*");

  case '/':
    if (n == '=') {
      advance();
      advance();
      return makePunct(TokenKind::DIV_ASSIGN, "/=");
    }
    advance();
    return makePunct(TokenKind::DIV, "/");

  case '%':
    if (n == '=') {
      advance();
      advance();
      return makePunct(TokenKind::MOD_ASSIGN, "%=");
    }
    advance();
    return makePunct(TokenKind::MOD, "%");

  case '&':
    if (n == '&') {
      advance();
      advance();
      return makePunct(TokenKind::AMPAMP, "&&");
    }
    if (n == '=') {
      advance();
      advance();
      return makePunct(TokenKind::AMP_ASSIGN, "&=");
    }
    advance();
    return makePunct(TokenKind::AMP, "&");

  case '|':
    if (n == '|') {
      advance();
      advance();
      return makePunct(TokenKind::PIPEPIPE, "||");
    }
    if (n == '=') {
      advance();
      advance();
      return makePunct(TokenKind::PIPE_ASSIGN, "|=");
    }
    advance();
    return makePunct(TokenKind::PIPE, "|");

  case '^':
    if (n == '=') {
      advance();
      advance();
      return makePunct(TokenKind::CARET_ASSIGN, "^=");
    }
    advance();
    return makePunct(TokenKind::CARET, "^");

  case '!':
    if (n == '=') {
      advance();
      advance();
      return makePunct(TokenKind::NEQ, "!=");
    }
    advance();
    return makePunct(TokenKind::BANG, "!");

  case '=':
    if (n == '=') {
      advance();
      advance();
      return makePunct(TokenKind::EQ, "==");
    }
    advance();
    return makePunct(TokenKind::ASSIGN, "=");

  case '<':
    if (n == '<') {
      if (look(2) == '=') {
        advance();
        advance();
        advance();
        return makePunct(TokenKind::LSHIFT_ASSIGN, "<<=");
      }
      advance();
      advance();
      return makePunct(TokenKind::LSHIFT, "<<");
    }
    if (n == '=') {
      advance();
      advance();
      return makePunct(TokenKind::LEQ, "<=");
    }
    advance();
    return makePunct(TokenKind::LT, "<");

  case '>':
    if (n == '>') {
      if (look(2) == '=') {
        advance();
        advance();
        advance();
        return makePunct(TokenKind::RSHIFT_ASSIGN, ">>=");
      }
      advance();
      advance();
      return makePunct(TokenKind::RSHIFT, ">>");
    }
    if (n == '=') {
      advance();
      advance();
      return makePunct(TokenKind::GEQ, ">=");
    }
    advance();
    return makePunct(TokenKind::GT, ">");

  default:
    advance();
    return Error("Unknown character");
  }
}

Token Lexer::next() {
  if (_hasPeek) {
    _hasPeek = false;
    return _peekTok;
  }

  skipWhiteSpaceAndComments();

  if (_pos >= _src.size())
    return make(TokenKind::END_OF_FILE, "");

  char c = cur();

  if (std::isalpha(c) || c == '_')
    return lexIdent();
  if (std::isdigit(c))
    return lexNumber();
  if (c == '\'')
    return lexChar();
  if (c == '"')
    return lexString();
  return lexPunct();
}

Token Lexer::peek() {
  if (!_hasPeek) {
    _peekTok = next();
    _hasPeek = true;
  }
  return _peekTok;
}

std::vector<Token> Lexer::tokenizeAll() {
  std::vector<Token> tokens;
  while (true) {
    Token t = next();
    tokens.push_back(t);
    if (t.isEof() || t.isError())
      break;
  }
  return tokens;
}
