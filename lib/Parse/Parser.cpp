#include "cc1/Parse/Parser.hpp"

Parser::Parser(TokenSource &lexer, ASTContext &ast, DiagnosticEngine &diag,
               const TargetInfo &target, const LangOptions &opts)
    : lex(lexer), ast(ast), types(ast.getTypes()), _diag(diag), target(target),
      opts(opts) {
  _cur = lex.next();
  _peek = lex.next();
}

Token Parser::advance() {
  Token prev = _cur;
  _cur = _peek;
  _peek = lex.next();
  return prev;
}

Token Parser::expect(TokenKind k, const char *msg) {
  if (!check(k)) {
    error(_cur.loc, msg);
    return _cur;
  }
  return advance();
}

bool Parser::check(TokenKind k) const { return _cur.kind == k; }

bool Parser::match(TokenKind k) {
  if (!check(k))
    return false;
  advance();
  return true;
}

bool Parser::isTypeName() const { return isTypeName(_cur); }

bool Parser::isTypeName(const Token &token) const {
  switch (token.kind) {
  case TokenKind::KW_VOID:
  case TokenKind::KW_CHAR:
  case TokenKind::KW_SHORT:
  case TokenKind::KW_INT:
  case TokenKind::KW_LONG:
  case TokenKind::KW_FLOAT:
  case TokenKind::KW_DOUBLE:
  case TokenKind::KW_SIGNED:
  case TokenKind::KW_UNSIGNED:
  case TokenKind::KW_STRUCT:
  case TokenKind::KW_UNION:
  case TokenKind::KW_ENUM:
  case TokenKind::KW_CONST:
  case TokenKind::KW_VOLATILE:
  case TokenKind::KW_AUTO:
  case TokenKind::KW_REGISTER:
  case TokenKind::KW_STATIC:
  case TokenKind::KW_EXTERN:
  case TokenKind::KW_TYPEDEF:
    return true;
  case TokenKind::IDENT:
    return _typedefs.count(token.text) > 0;
  default:
    return false;
  }
}

void Parser::error(const SourceLoc &loc, const std::string &msg) {
  _diag.error(loc, msg);
}

void Parser::syncToNextDecl() {
  while (!_cur.isEof()) {
    if (check(TokenKind::SEMICOLON)) {
      advance();
      return;
    }
    if (isTypeName())
      return;
    advance();
  }
}

TranslationUnit *Parser::parse() {
  TranslationUnit *tu = ast.create<TranslationUnit>();
  tu->loc = _cur.loc;

  while (!_cur.isEof() && !_diag.hasErrors()) {
    if (check(TokenKind::SEMICOLON)) {
      advance();
      continue;
    }
    Decl *d = parseDecl();
    if (d)
      tu->decls.push_back(d);
    else
      syncToNextDecl();
  }
  return tu;
}
