#include "Parser.hpp"
#include "AST.hpp"
#include "Type.hpp"

Parser::Parser(Lexer &lexer, TypePool &types, Diagnostic &diag,
               const TargetInfo &target, const LangOptions &opts)
    : lex(lexer), types(types), _diag(diag), target(target), opts(opts) {
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

bool Parser::isTypeName() const {
  switch (_cur.kind) {
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
    return _typedefs.count(_cur.text) > 0;
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
  TranslationUnit *tu = new TranslationUnit();
  tu->loc = _cur.loc;

  while (!_cur.isEof()) {
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

Decl *Parser::parseDecl() {
  SourceLoc loc = _cur.loc;

  StorageClass sc = parseStorageClass();
  Qualifiers quals = parseQualifiers();
  Type *base = parseTypeSpec();

  if (!base) {
    error(loc, "expected type specifier");
    return nullptr;
  }

  base->quals = quals;

  if (sc == StorageClass::TYPEDEF) {
    std::string name;
    Type *full = parseDeclarator(base, name);
    if (name.empty()) {
      error(_cur.loc, "typedef requires a name");
      return nullptr;
    }
    expect(TokenKind::SEMICOLON, "expected ';' after typedef");

    TypedefDecl *td = new TypedefDecl();
    td->loc = loc;
    td->name = name;
    td->type = full;
    td->underlying = full;
    _typedefs[name] = full;
    return td;
  }
  std::string name;
  Type *full = parseDeclarator(base, name);

  if (name.empty()) {
    expect(TokenKind::SEMICOLON, "expected ';'");
    return nullptr;
  }

  if (full->isFunction())
    return parseFuncDecl(full, name, loc);

  VarDecl *first = parseVarDecl(full, name, loc);
  first->sc = sc;

  expect(TokenKind::SEMICOLON, "expected ';' after declaration");
  return first;
}

StorageClass Parser::parseStorageClass() {
  switch (_cur.kind) {
  case TokenKind::KW_AUTO:
    advance();
    return StorageClass::AUTO;
  case TokenKind::KW_REGISTER:
    advance();
    return StorageClass::REGISTER;
  case TokenKind::KW_STATIC:
    advance();
    return StorageClass::STATIC;
  case TokenKind::KW_EXTERN:
    advance();
    return StorageClass::EXTERN;
  case TokenKind::KW_TYPEDEF:
    advance();
    return StorageClass::TYPEDEF;
  default:
    return StorageClass::NONE;
  }
}

Qualifiers Parser::parseQualifiers() {
  Qualifiers q;
  while (true) {
    if (match(TokenKind::KW_CONST)) {
      q.isConst = true;
      continue;
    }
    if (match(TokenKind::KW_VOLATILE)) {
      q.isVolatile = true;
      continue;
    }
    break;
  }
  return q;
}

Type *Parser::parseTypeSpec() {
  if (check(TokenKind::KW_STRUCT) || check(TokenKind::KW_UNION))
    return parseStructOrUnion();
  if (check(TokenKind::KW_ENUM))
    return parseEnum();
  if (check(TokenKind::IDENT) && _typedefs.count(_cur.text)) {
    Type *t = _typedefs[_cur.text];
    advance();
    return t;
  }

  bool isSigned = false;
  bool isUnsigned = false;
  bool isShort = false;
  bool isLong = false;
  bool isLongLong = false;
  bool hasChar = false;
  bool hasInt = false;
  bool hasFloat = false;
  bool hasDouble = false;
  bool hasVoid = false;

  bool any = false;

  while (true) {
    switch (_cur.kind) {
    case TokenKind::KW_VOID:
      hasVoid = true;
      any = true;
      advance();
      break;
    case TokenKind::KW_CHAR:
      hasChar = true;
      any = true;
      advance();
      break;
    case TokenKind::KW_INT:
      hasInt = true;
      any = true;
      advance();
      break;
    case TokenKind::KW_FLOAT:
      hasFloat = true;
      any = true;
      advance();
      break;
    case TokenKind::KW_DOUBLE:
      hasDouble = true;
      any = true;
      advance();
      break;
    case TokenKind::KW_SIGNED:
      isSigned = true;
      any = true;
      advance();
      break;
    case TokenKind::KW_UNSIGNED:
      isUnsigned = true;
      any = true;
      advance();
      break;
    case TokenKind::KW_SHORT:
      isShort = true;
      any = true;
      advance();
      break;
    case TokenKind::KW_LONG:
      any = true;
      advance();
      if (check(TokenKind::KW_LONG)) {
        isLongLong = true;
        advance();
      } else {
        isLong = true;
      }
      break;
    default:
      goto done;
    }
  }
done:
  if (!any)
    return nullptr;
  if (hasVoid)
    return types.voidTy();
  if (hasFloat)
    return types.floatTy();
  if (hasDouble && isLong)
    return types.make(Type::LONGDOUBLE);
  if (hasDouble)
    return types.doubleTy();
  if (hasChar)
    return types.make(isUnsigned ? Type::UCHAR : Type::SCHAR);
  if (isShort)
    return types.make(isUnsigned ? Type::USHORT : Type::SHORT);
  if (isLongLong)
    return types.make(isUnsigned ? Type::ULONGLONG : Type::LONGLONG);
  if (isLong)
    return types.make(isUnsigned ? Type::ULONG : Type::LONG);
  if (isUnsigned)
    return types.uintTy();
  if (hasInt || isSigned)
    return types.intTy(); // ← utilise les deux
  return types.intTy();
}

Type *Parser::parsePointer(Type *base) {
  while (match(TokenKind::MUL)) {
    Qualifiers q = parseQualifiers();
    base = types.ptrTo(base);
    base->quals = q;
  }
  return base;
}

void Parser::replacePlaceholder(Type *node, Type *placeholder,
                                Type *replacement) {
  if (!node)
    return;
  if (node->pointee == placeholder) {
    node->pointee = replacement;
    return;
  }
  if (node->elemType == placeholder) {
    node->elemType = replacement;
    return;
  }
  if (node->retType == placeholder) {
    node->retType = replacement;
    return;
  }
  replacePlaceholder(node->pointee, placeholder, replacement);
  replacePlaceholder(node->elemType, placeholder, replacement);
  replacePlaceholder(node->retType, placeholder, replacement);
}

Type *Parser::parseDeclarator(Type *base, std::string &nameOut) {
  base = parsePointer(base);

  if (check(TokenKind::L_PAREN) && !isTypeName() &&
      _peek.kind != TokenKind::R_PAREN) {

    advance();

    Type *inner = nullptr;
    std::string innerName;

    Type *placeholder = types.make(Type::INT);
    inner = parseDeclarator(placeholder, innerName);
    nameOut = innerName;

    expect(TokenKind::R_PAREN, "expected ')' in declarator");

    base = parseSuffix(base);

    replacePlaceholder(inner, placeholder, base);
    return inner;
  }

  if (check(TokenKind::IDENT)) {
    nameOut = _cur.text;
    advance();
  }

  return parseSuffix(base);
}

Type *Parser::parseSuffix(Type *base) {
  while (true) {
    if (check(TokenKind::L_BRACKET)) {
      base = parseArrayType(base);
    } else if (check(TokenKind::L_PAREN)) {
      base = parseFuncType(base);
    } else {
      break;
    }
  }
  return base;
}

Type *Parser::parseArrayType(Type *elemType) {
  expect(TokenKind::L_BRACKET, "expected '['");
  int size = -1;
  if (!check(TokenKind::R_BRACKET)) {
    Expr *e = parseExpr();
    if (auto *lit = dynamic_cast<IntLitExpr *>(e))
      size = (int)lit->val;
  }
  expect(TokenKind::R_BRACKET, "expected ']'");
  return Type::makeArray(elemType, size);
}

Type *Parser::parseFuncType(Type *retType) {
  expect(TokenKind::L_PAREN, "expected '('");
  std::vector<Type *> params;
  bool variadic = false;

  if (!check(TokenKind::R_PAREN)) {
    if (check(TokenKind::KW_VOID) && _peek.kind == TokenKind::R_PAREN) {
      advance();
    } else {
      do {
        if (check(TokenKind::ELLIPSIS)) {
          advance();
          variadic = true;
          break;
        }
        Type *ptype = parseTypeSpec();
        Qualifiers pq = parseQualifiers();
        ptype->quals = pq;
        std::string pname;
        ptype = parseDeclarator(ptype, pname);
        params.push_back(ptype);
      } while (match(TokenKind::COMMA));
    }
  }

  expect(TokenKind::R_PAREN, "expected ')'");
  return Type::makeFunction(retType, std::move(params), variadic);
}

FuncDecl *Parser::parseFuncDecl(Type *retType, const std::string &name,
                                SourceLoc &loc) {
  FuncDecl *fn = new FuncDecl();
  fn->loc = loc;
  fn->name = name;
  fn->type = retType;

  if (match(TokenKind::L_BRACE)) {
    error(_cur.loc, "function definitions are not implemented yet");
    int depth = 1;
    while (depth > 0 && !_cur.isEof()) {
      if (match(TokenKind::L_BRACE)) {
        ++depth;
      } else if (match(TokenKind::R_BRACE)) {
        --depth;
      } else {
        advance();
      }
    }
  } else {
    expect(TokenKind::SEMICOLON, "expected ';' after function declaration");
  }

  return fn;
}

VarDecl *Parser::parseVarDecl(Type *baseType, const std::string &name,
                              SourceLoc &loc) {
  VarDecl *var = new VarDecl();
  var->loc = loc;
  var->name = name;
  var->type = baseType;
  var->isGlobal = true;

  if (match(TokenKind::ASSIGN)) {
    error(_cur.loc, "initializers are not implemented yet");
    while (!_cur.isEof() && !check(TokenKind::COMMA) &&
           !check(TokenKind::SEMICOLON)) {
      advance();
    }
  }

  if (match(TokenKind::COMMA)) {
    error(_cur.loc, "multiple declarators are not supported yet");
    while (!_cur.isEof() && !check(TokenKind::SEMICOLON))
      advance();
  }

  return var;
}

Type *Parser::parseStructOrUnion() {
  bool isStruct = check(TokenKind::KW_STRUCT);
  advance();

  Type *t = types.make(isStruct ? Type::STRUCT : Type::UNION);

  if (check(TokenKind::IDENT)) {
    t->tag = _cur.text;
    advance();
  }

  if (match(TokenKind::L_BRACE)) {
    error(_cur.loc, "struct/union definitions are not implemented yet");
    int depth = 1;
    while (depth > 0 && !_cur.isEof()) {
      if (match(TokenKind::L_BRACE)) {
        ++depth;
      } else if (match(TokenKind::R_BRACE)) {
        --depth;
      } else {
        advance();
      }
    }
  }

  return t;
}

Type *Parser::parseEnum() {
  expect(TokenKind::KW_ENUM, "expected 'enum'");
  Type *t = types.make(Type::ENUM);

  if (check(TokenKind::IDENT)) {
    t->tag = _cur.text;
    advance();
  }

  if (match(TokenKind::L_BRACE)) {
    error(_cur.loc, "enum definitions are not implemented yet");
    int depth = 1;
    while (depth > 0 && !_cur.isEof()) {
      if (match(TokenKind::L_BRACE)) {
        ++depth;
      } else if (match(TokenKind::R_BRACE)) {
        --depth;
      } else {
        advance();
      }
    }
  }

  return t;
}
