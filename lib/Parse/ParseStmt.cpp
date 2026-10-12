#include "cc1/Parse/Parser.hpp"

ReturnStmt *Parser::parseReturnStmt() {
  auto *stmt = ast.create<ReturnStmt>();
  stmt->loc = expect(TokenKind::KW_RETURN, "expected 'return'").loc;

  if (!check(TokenKind::SEMICOLON))
    stmt->value = parseExpr();

  if (!_diag.hasErrors())
    expect(TokenKind::SEMICOLON, "expected ';' after return");

  return stmt;
}

Stmt *Parser::parseStmt() {
  if (check(TokenKind::KW_IF))
    return parseIfStmt();
  if (check(TokenKind::KW_WHILE))
    return parseWhileStmt();
  if (check(TokenKind::KW_FOR))
    return parseForStmt();
  if (check(TokenKind::L_BRACE))
    return parseCompoundStmt();

  if (check(TokenKind::KW_RETURN))
    return parseReturnStmt();
  if (check(TokenKind::KW_BREAK)) {
    auto *stmt = ast.create<BreakStmt>();
    stmt->loc = advance().loc;
    expect(TokenKind::SEMICOLON, "expected ';' after break");
    if (_diag.hasErrors())
      return nullptr;
    return stmt;
  }
  if (check(TokenKind::KW_CONTINUE)) {
    auto *stmt = ast.create<ContinueStmt>();
    stmt->loc = advance().loc;
    expect(TokenKind::SEMICOLON, "expected ';' after continue");
    if (_diag.hasErrors())
      return nullptr;
    return stmt;
  }
  auto *stmt = ast.create<ExprStmt>();
  stmt->loc = _cur.loc;

  if (match(TokenKind::SEMICOLON))
    return stmt;

  stmt->expr = parseExpr();
  if (!_diag.hasErrors())
    expect(TokenKind::SEMICOLON, "expected ';' after expression");

  return stmt;
}

CompoundStmt *Parser::parseCompoundStmt() {
  auto *block = ast.create<CompoundStmt>();
  block->loc = expect(TokenKind::L_BRACE, "expected '{'").loc;
  auto savedTypedefs = _typedefs;

  while (!check(TokenKind::R_BRACE) && !_cur.isEof() && !_diag.hasErrors()) {
    Node *item = isTypeName() ? static_cast<Node *>(parseDecl(false))
                              : static_cast<Node *>(parseStmt());
    if (item)
      block->items.push_back(item);
    else
      break;
  }

  if (!_diag.hasErrors())
    expect(TokenKind::R_BRACE, "expected '}' at end of block");

  _typedefs = std::move(savedTypedefs);

  return block;
}

IfStmt *Parser::parseIfStmt() {
  auto *stmt = ast.create<IfStmt>();
  stmt->loc = expect(TokenKind::KW_IF, "expected 'if'").loc;

  expect(TokenKind::L_PAREN, "expected '(' after if keyword");
  if (_diag.hasErrors())
    return nullptr;

  stmt->cond = parseExpr();
  if (!stmt->cond || _diag.hasErrors())
    return nullptr;

  expect(TokenKind::R_PAREN, "expected ')' after if condition");
  if (_diag.hasErrors())
    return nullptr;

  stmt->then = parseStmt();
  if (!stmt->then || _diag.hasErrors())
    return nullptr;

  if (match(TokenKind::KW_ELSE)) {
    stmt->els = parseStmt();
    if (!stmt->els || _diag.hasErrors())
      return nullptr;
  }

  return stmt;
}

WhileStmt *Parser::parseWhileStmt() {
  auto *stmt = ast.create<WhileStmt>();
  stmt->loc = expect(TokenKind::KW_WHILE, "expected while").loc;

  expect(TokenKind::L_PAREN, "expected '(' after while loop condition");
  if (_diag.hasErrors())
    return nullptr;

  stmt->cond = parseExpr();
  if (!stmt->cond || _diag.hasErrors())
    return nullptr;

  expect(TokenKind::R_PAREN, "expected ')' after while loop conditions");
  if (_diag.hasErrors())
    return nullptr;

  stmt->body = parseStmt();
  if (!stmt->body || _diag.hasErrors())
    return nullptr;

  return stmt;
}

ForStmt *Parser::parseForStmt() {
  auto *stmt = ast.create<ForStmt>();
  stmt->loc = expect(TokenKind::KW_FOR, "expected 'for'").loc;
  auto savedTypedefs = _typedefs;
  auto fail = [&]() -> ForStmt * {
    _typedefs = std::move(savedTypedefs);
    return nullptr;
  };

  expect(TokenKind::L_PAREN, "expected '(' after for");
  if (_diag.hasErrors())
    return fail();

  if (isTypeName()) {
    // parseDecl consumes the first semicolon itself.
    stmt->init = parseDecl(false);
    if (!stmt->init || _diag.hasErrors())
      return fail();
  } else {
    if (!check(TokenKind::SEMICOLON)) {
      auto *init = ast.create<ExprStmt>();
      init->loc = _cur.loc;
      init->expr = parseExpr();
      stmt->init = init;
      if (!init->expr || _diag.hasErrors())
        return fail();
    }
    expect(TokenKind::SEMICOLON, "expected ';' after for initializer");
    if (_diag.hasErrors())
      return fail();
  }

  if (!check(TokenKind::SEMICOLON)) {
    stmt->cond = parseExpr();
    if (!stmt->cond || _diag.hasErrors())
      return fail();
  }
  expect(TokenKind::SEMICOLON, "expected ';' after for condition");
  if (_diag.hasErrors())
    return fail();

  if (!check(TokenKind::R_PAREN)) {
    stmt->incr = parseExpr();
    if (!stmt->incr || _diag.hasErrors())
      return fail();
  }
  expect(TokenKind::R_PAREN, "expected ')' after for increment");
  if (_diag.hasErrors())
    return fail();

  stmt->body = parseStmt();
  if (!stmt->body || _diag.hasErrors())
    return fail();

  _typedefs = std::move(savedTypedefs);
  return stmt;
}
