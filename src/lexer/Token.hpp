#pragma once
#include <cstdint>
#include <string>

enum class TokenKind {

  LITERAL_BEGIN,
  INT_LIT,    // 42 lol
  FLOAT_LIT,  // 3.14 (but pi = 3, because note physics related here yo)
  CHAR_LIT,   // 'a'
  STRING_LIT, // "hello there"
  LITERAL_END,

  IDENT, // foo, my_var, ...

  KEYWORD_BEGIN,
  KW_AUTO,     // auto
  KW_BREAK,    // break
  KW_CASE,     // case
  KW_CHAR,     // char
  KW_CONST,    // const
  KW_CONTINUE, // continue
  KW_DEFAULT,  // default
  KW_DO,       // do
  KW_DOUBLE,   // double
  KW_ELSE,     // else
  KW_ENUM,     // enum
  KW_EXTERN,   // extern
  KW_FLOAT,    // float
  KW_FOR,      // for
  KW_GOTO,     // goto
  KW_IF,       // if
  KW_INT,      // int
  KW_LONG,     // long
  KW_REGISTER, // register
  KW_RETURN,   // return
  KW_SHORT,    // short
  KW_SIGNED,   // signed
  KW_SIZEOF,   // sizeof
  KW_STATIC,   // static
  KW_STRUCT,   // struct
  KW_SWITCH,   // switch
  KW_TYPEDEF,  // typedef
  KW_UNION,    // union
  KW_UNSIGNED, // unsigned
  KW_VOID,     // void
  KW_VOLATILE, // volatile
  KW_WHILE,    // while
  KEYWORD_END,

  PUNCT_BEGIN,
  L_PAREN,   // (
  R_PAREN,   // )
  L_BRACE,   // {
  R_BRACE,   // }
  L_BRACKET, // [
  R_BRACKET, // ]
  SEMICOLON, // ;
  COLON,     // :
  COMMA,     // ,
  DOT,       // .
  HASH,      // #
  HASHHASH,  // ##
  ARROW,     // ->
  ELLIPSIS,  // ...
  PUNCT_END,

  OP_BEGIN,

  PLUS,  // +
  MINUS, // -
  MUL,   // *
  DIV,   // /
  MOD,   // %

  AMP,    // &
  PIPE,   // |
  CARET,  // ^
  TILDE,  // ~
  LSHIFT, //
  RSHIFT, // >>

  BANG,     // !
  AMPAMP,   // &&
  PIPEPIPE, // ||

  EQ,  // ==
  NEQ, // !=
  LT,  //
  GT,  // >
  LEQ, // <=
  GEQ, // >=

  ASSIGN,        // =
  PLUS_ASSIGN,   // +=
  MINUS_ASSIGN,  // -=
  MUL_ASSIGN,    // *=
  DIV_ASSIGN,    // /=
  MOD_ASSIGN,    // %=
  AMP_ASSIGN,    // &=
  PIPE_ASSIGN,   // |=
  CARET_ASSIGN,  // ^=
  LSHIFT_ASSIGN, // <<=
  RSHIFT_ASSIGN, // >>=

  PLUSPLUS,   // ++
  MINUSMINUS, // --

  QUESTION, // ?

  OP_END,

  END_OF_FILE,
  ERROR,
};

inline bool isLiteral(TokenKind k) {
  return k > TokenKind::LITERAL_BEGIN && k < TokenKind::LITERAL_END;
}
inline bool isKeyword(TokenKind k) {
  return k > TokenKind::KEYWORD_BEGIN && k < TokenKind::KEYWORD_END;
}
inline bool isPunct(TokenKind k) {
  return k > TokenKind::PUNCT_BEGIN && k < TokenKind::PUNCT_END;
}
inline bool isOperator(TokenKind k) {
  return k > TokenKind::OP_BEGIN && k < TokenKind::OP_END;
}
struct SourceLoc {
  const char *filename; /* no ptr copy eheh */
  uint32_t line;
  uint32_t col;
};

struct Token {
  TokenKind kind;
  std::string text;
  SourceLoc loc;

  union {
    long long int_val;
    double float_val;
    char char_val;
  };
  std::string str_val;

  Token()
      : kind(TokenKind::ERROR), int_val(0) {
  } /* zero-init int_val zeros all union members, they share the same memory
       (int_val is a long long so the bigger type)*/

  Token(TokenKind k, std::string t, SourceLoc l)
      : kind(k), text(std::move(t)), loc(l), int_val(0) {}

  bool is(TokenKind k) const { return kind == k; };
  bool isKeyword() const { return ::isKeyword(kind); }
  bool isLiteral() const { return ::isLiteral(kind); }
  bool isEof() const { return kind == TokenKind::END_OF_FILE; }
  bool isError() const { return kind == TokenKind::ERROR; }
};

const char *tokenKindName(TokenKind k);
