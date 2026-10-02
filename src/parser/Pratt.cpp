#include "AST.hpp"
#include "Parser.hpp"

namespace {
int ternaryPrec() { return 3; }
} // namespace

Expr *Parser::parseExpr(int minPrec) {
  Expr *lhs = parseUnary();
  if (!lhs)
    return nullptr;

  while (true) {
    if (check(TokenKind::QUESTION)) {
      if (ternaryPrec() < minPrec)
        break;
      auto *tern = new TernaryExpr();
      tern->loc = lhs->loc;
      tern->cond = lhs;
      advance();
      tern->then = parseExpr();
      expect(TokenKind::COLON, "expected ':' in conditional expression");
      tern->els = parseExpr(ternaryPrec());
      lhs = tern;
      continue;
    }

    int prec = infixPrec(_cur.kind);
    if (prec < minPrec)
      break;

    TokenKind opTok = _cur.kind;
    advance();

    int nextMinPrec = isRightAssoc(opTok) ? prec : prec + 1;
    Expr *rhs = parseExpr(nextMinPrec);
    if (!rhs)
      return nullptr;

    auto *bin = new BinaryExpr();
    bin->loc = lhs->loc;
    bin->op = tokenToBinaryOp(opTok);
    bin->lhs = lhs;
    bin->rhs = rhs;
    lhs = bin;
  }

  return lhs;
}

Expr *Parser::parseUnary() {
  Token tok = _cur;
  switch (tok.kind) {
  case TokenKind::PLUSPLUS: {
    advance();
    Expr *operand = parseUnary();
    if (!operand)
      return nullptr;
    auto *node = new UnaryExpr();
    node->loc = tok.loc;
    node->op = UnaryOp::PRE_INC;
    node->operand = operand;
    return node;
  }
  case TokenKind::MINUSMINUS: {
    advance();
    Expr *operand = parseUnary();
    if (!operand)
      return nullptr;
    auto *node = new UnaryExpr();
    node->loc = tok.loc;
    node->op = UnaryOp::PRE_DEC;
    node->operand = operand;
    return node;
  }
  case TokenKind::PLUS: {
    advance();
    return parseUnary();
  }
  case TokenKind::MINUS: {
    advance();
    Expr *operand = parseUnary();
    if (!operand)
      return nullptr;
    auto *node = new UnaryExpr();
    node->loc = tok.loc;
    node->op = UnaryOp::NEG;
    node->operand = operand;
    return node;
  }
  case TokenKind::BANG: {
    advance();
    Expr *operand = parseUnary();
    if (!operand)
      return nullptr;
    auto *node = new UnaryExpr();
    node->loc = tok.loc;
    node->op = UnaryOp::NOT;
    node->operand = operand;
    return node;
  }
  case TokenKind::TILDE: {
    advance();
    Expr *operand = parseUnary();
    if (!operand)
      return nullptr;
    auto *node = new UnaryExpr();
    node->loc = tok.loc;
    node->op = UnaryOp::BITNOT;
    node->operand = operand;
    return node;
  }
  case TokenKind::MUL: {
    advance();
    Expr *operand = parseUnary();
    if (!operand)
      return nullptr;
    auto *node = new UnaryExpr();
    node->loc = tok.loc;
    node->op = UnaryOp::DEREF;
    node->operand = operand;
    return node;
  }
  case TokenKind::AMP: {
    advance();
    Expr *operand = parseUnary();
    if (!operand)
      return nullptr;
    auto *node = new UnaryExpr();
    node->loc = tok.loc;
    node->op = UnaryOp::ADDR;
    node->operand = operand;
    return node;
  }
  case TokenKind::KW_SIZEOF: {
    advance();
    auto *sz = new SizeofExpr();
    sz->loc = tok.loc;
    if (match(TokenKind::L_PAREN)) {
      if (isTypeName()) {
        Type *ty = parseTypeSpec();
        if (!ty) {
          error(_cur.loc, "expected type name after sizeof(");
        } else {
          Qualifiers q = parseQualifiers();
          ty->quals = q;
          std::string dummy;
          ty = parseDeclarator(ty, dummy);
          sz->ofType = true;
          sz->type = ty;
        }
      } else {
        sz->expr = parseExpr();
      }
      expect(TokenKind::R_PAREN, "expected ')' after sizeof");
    } else {
      sz->expr = parseUnary();
    }
    return sz;
  }
  default: {
    Expr *primary = parsePrimary();
    if (!primary)
      return nullptr;
    return parsePostfix(primary);
  }
  }
}

Expr *Parser::parsePrimary() {
  Token tok = _cur;
  switch (tok.kind) {
  case TokenKind::INT_LIT: {
    auto *lit = new IntLitExpr();
    lit->loc = tok.loc;
    lit->val = tok.int_val;
    advance();
    return lit;
  }
  case TokenKind::FLOAT_LIT: {
    auto *lit = new FloatLitExpr();
    lit->loc = tok.loc;
    lit->val = tok.float_val;
    advance();
    return lit;
  }
  case TokenKind::CHAR_LIT: {
    auto *lit = new CharLitExpr();
    lit->loc = tok.loc;
    lit->val = tok.char_val;
    advance();
    return lit;
  }
  case TokenKind::STRING_LIT: {
    auto *lit = new StringLitExpr();
    lit->loc = tok.loc;
    lit->val = tok.str_val;
    advance();
    return lit;
  }
  case TokenKind::IDENT: {
    auto *id = new IdentExpr();
    id->loc = tok.loc;
    id->name = tok.text;
    advance();
    return id;
  }
  case TokenKind::L_PAREN: {
    advance();
    Expr *expr = parseExpr();
    expect(TokenKind::R_PAREN, "expected ')' after expression");
    return expr;
  }
  default:
    error(tok.loc, "expected expression");
    return nullptr;
  }
}

Expr *Parser::parsePostfix(Expr *base) {
  if (!base)
    return nullptr;

  while (true) {
    if (match(TokenKind::L_PAREN)) {
      auto *call = new CallExpr();
      call->loc = base->loc;
      call->callee = base;
      if (!check(TokenKind::R_PAREN)) {
        do {
          Expr *arg = parseExpr();
          call->args.push_back(arg);
        } while (match(TokenKind::COMMA));
      }
      expect(TokenKind::R_PAREN, "expected ')' in call expression");
      base = call;
      continue;
    }
    if (match(TokenKind::L_BRACKET)) {
      auto *idx = new IndexExpr();
      idx->loc = base->loc;
      idx->base = base;
      idx->index = parseExpr();
      expect(TokenKind::R_BRACKET, "expected ']' in index expression");
      base = idx;
      continue;
    }
    if (check(TokenKind::DOT) || check(TokenKind::ARROW)) {
      bool arrow = check(TokenKind::ARROW);
      advance();
      if (!check(TokenKind::IDENT)) {
        error(_cur.loc, "expected identifier after member access");
        return base;
      }
      auto *mem = new MemberExpr();
      mem->loc = base->loc;
      mem->base = base;
      mem->arrow = arrow;
      mem->member = _cur.text;
      advance();
      base = mem;
      continue;
    }
    if (check(TokenKind::PLUSPLUS) || check(TokenKind::MINUSMINUS)) {
      TokenKind opTok = _cur.kind;
      advance();
      auto *node = new UnaryExpr();
      node->loc = base->loc;
      node->operand = base;
      node->op = (opTok == TokenKind::PLUSPLUS) ? UnaryOp::POST_INC
                                                : UnaryOp::POST_DEC;
      base = node;
      continue;
    }
    break;
  }

  return base;
}

Expr *Parser::parseCast() { return parseUnary(); }

int Parser::infixPrec(TokenKind k) const {
  switch (k) {
  case TokenKind::COMMA:
    return 1;
  case TokenKind::ASSIGN:
  case TokenKind::PLUS_ASSIGN:
  case TokenKind::MINUS_ASSIGN:
  case TokenKind::MUL_ASSIGN:
  case TokenKind::DIV_ASSIGN:
  case TokenKind::MOD_ASSIGN:
  case TokenKind::AMP_ASSIGN:
  case TokenKind::PIPE_ASSIGN:
  case TokenKind::CARET_ASSIGN:
  case TokenKind::LSHIFT_ASSIGN:
  case TokenKind::RSHIFT_ASSIGN:
    return 2;
  case TokenKind::PIPEPIPE:
    return 4;
  case TokenKind::AMPAMP:
    return 5;
  case TokenKind::PIPE:
    return 6;
  case TokenKind::CARET:
    return 7;
  case TokenKind::AMP:
    return 8;
  case TokenKind::EQ:
  case TokenKind::NEQ:
    return 9;
  case TokenKind::LT:
  case TokenKind::LEQ:
  case TokenKind::GT:
  case TokenKind::GEQ:
    return 10;
  case TokenKind::LSHIFT:
  case TokenKind::RSHIFT:
    return 11;
  case TokenKind::PLUS:
  case TokenKind::MINUS:
    return 12;
  case TokenKind::MUL:
  case TokenKind::DIV:
  case TokenKind::MOD:
    return 13;
  default:
    return -1;
  }
}

bool Parser::isRightAssoc(TokenKind k) const {
  switch (k) {
  case TokenKind::ASSIGN:
  case TokenKind::PLUS_ASSIGN:
  case TokenKind::MINUS_ASSIGN:
  case TokenKind::MUL_ASSIGN:
  case TokenKind::DIV_ASSIGN:
  case TokenKind::MOD_ASSIGN:
  case TokenKind::AMP_ASSIGN:
  case TokenKind::PIPE_ASSIGN:
  case TokenKind::CARET_ASSIGN:
  case TokenKind::LSHIFT_ASSIGN:
  case TokenKind::RSHIFT_ASSIGN:
    return true;
  default:
    return false;
  }
}

BinaryOp Parser::tokenToBinaryOp(TokenKind k) const {
  switch (k) {
  case TokenKind::PLUS:
    return BinaryOp::ADD;
  case TokenKind::MINUS:
    return BinaryOp::SUB;
  case TokenKind::MUL:
    return BinaryOp::MUL;
  case TokenKind::DIV:
    return BinaryOp::DIV;
  case TokenKind::MOD:
    return BinaryOp::MOD;
  case TokenKind::PLUS_ASSIGN:
    return BinaryOp::ADD_ASSIGN;
  case TokenKind::MINUS_ASSIGN:
    return BinaryOp::SUB_ASSIGN;
  case TokenKind::MUL_ASSIGN:
    return BinaryOp::MUL_ASSIGN;
  case TokenKind::DIV_ASSIGN:
    return BinaryOp::DIV_ASSIGN;
  case TokenKind::MOD_ASSIGN:
    return BinaryOp::MOD_ASSIGN;
  case TokenKind::AMP:
    return BinaryOp::BITAND;
  case TokenKind::PIPE:
    return BinaryOp::BITOR;
  case TokenKind::CARET:
    return BinaryOp::BITXOR;
  case TokenKind::AMP_ASSIGN:
    return BinaryOp::AND_ASSIGN;
  case TokenKind::PIPE_ASSIGN:
    return BinaryOp::OR_ASSIGN;
  case TokenKind::CARET_ASSIGN:
    return BinaryOp::XOR_ASSIGN;
  case TokenKind::LSHIFT:
    return BinaryOp::LSHIFT;
  case TokenKind::RSHIFT:
    return BinaryOp::RSHIFT;
  case TokenKind::LSHIFT_ASSIGN:
    return BinaryOp::LSHIFT_ASSIGN;
  case TokenKind::RSHIFT_ASSIGN:
    return BinaryOp::RSHIFT_ASSIGN;
  case TokenKind::AMPAMP:
    return BinaryOp::AND;
  case TokenKind::PIPEPIPE:
    return BinaryOp::OR;
  case TokenKind::EQ:
    return BinaryOp::EQ;
  case TokenKind::NEQ:
    return BinaryOp::NEQ;
  case TokenKind::LT:
    return BinaryOp::LT;
  case TokenKind::GT:
    return BinaryOp::GT;
  case TokenKind::LEQ:
    return BinaryOp::LEQ;
  case TokenKind::GEQ:
    return BinaryOp::GEQ;
  case TokenKind::ASSIGN:
    return BinaryOp::ASSIGN;
  case TokenKind::COMMA:
    return BinaryOp::COMMA;
  default:
    return BinaryOp::ADD;
  }
}
