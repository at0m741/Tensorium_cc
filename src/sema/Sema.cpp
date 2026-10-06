#include "Sema.hpp"
#include <algorithm>
#include <cctype>
#include <limits>

namespace {
bool assignment(BinaryOp op) {
  return op >= BinaryOp::ASSIGN && op <= BinaryOp::RSHIFT_ASSIGN;
}
BinaryOp compoundOperation(BinaryOp op) {
  switch (op) {
  case BinaryOp::ADD_ASSIGN:
    return BinaryOp::ADD;
  case BinaryOp::SUB_ASSIGN:
    return BinaryOp::SUB;
  case BinaryOp::MUL_ASSIGN:
    return BinaryOp::MUL;
  case BinaryOp::DIV_ASSIGN:
    return BinaryOp::DIV;
  case BinaryOp::MOD_ASSIGN:
    return BinaryOp::MOD;
  case BinaryOp::AND_ASSIGN:
    return BinaryOp::BITAND;
  case BinaryOp::OR_ASSIGN:
    return BinaryOp::BITOR;
  case BinaryOp::XOR_ASSIGN:
    return BinaryOp::BITXOR;
  case BinaryOp::LSHIFT_ASSIGN:
    return BinaryOp::LSHIFT;
  case BinaryOp::RSHIFT_ASSIGN:
    return BinaryOp::RSHIFT;
  default:
    return op;
  }
}
int rank(Type *t) {
  switch (t->kind) {
  case Type::BOOL:
    return 0;
  case Type::CHAR:
  case Type::SCHAR:
  case Type::UCHAR:
    return 1;
  case Type::SHORT:
  case Type::USHORT:
    return 2;
  case Type::INT:
  case Type::UINT:
  case Type::ENUM:
    return 3;
  case Type::LONG:
  case Type::ULONG:
    return 4;
  case Type::LONGLONG:
  case Type::ULONGLONG:
    return 5;
  default:
    return -1;
  }
}
} // namespace

Type *Sema::fail(SourceLoc loc, const std::string &message) {
  diag.error(loc, message);
  return nullptr;
}

Type *Sema::unqualified(Type *t) {
  return t->quals.isConst || t->quals.isVolatile ? types.qualified(t, {}) : t;
}

Type *Sema::promoted(Type *t) {
  if (t->isIntegral() && rank(t) < 3)
    return types.intTy();
  if (t->kind == Type::ENUM)
    return types.intTy();
  return unqualified(t);
}

Type *Sema::sizeType(bool isSigned) {
  if (target.sizeofPointer == target.sizeofLong)
    return types.make(isSigned ? Type::LONG : Type::ULONG);
  return types.make(isSigned ? Type::INT : Type::UINT);
}

Type *Sema::integerLiteralType(const IntLitExpr *e) {
  const auto &text = e->spelling;
  const bool hex =
      text.size() > 2 && text[0] == '0' && (text[1] == 'x' || text[1] == 'X');
  const bool nondecimal = text.size() > 1 && text[0] == '0';
  size_t end = hex ? 2 : 0;
  while (end < text.size() &&
         (hex ? std::isxdigit(static_cast<unsigned char>(text[end]))
              : std::isdigit(static_cast<unsigned char>(text[end]))))
    ++end;
  std::string suffix = text.substr(end);
  if (suffix.find("lL") != std::string::npos ||
      suffix.find("Ll") != std::string::npos)
    return fail(e->loc, "invalid integer literal suffix");
  for (auto &ch : suffix)
    ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  std::vector<Type::Kind> candidates;
  if (suffix.empty())
    candidates =
        nondecimal
            ? std::vector<Type::Kind>{Type::INT,      Type::UINT,
                                      Type::LONG,     Type::ULONG,
                                      Type::LONGLONG, Type::ULONGLONG}
            : std::vector<Type::Kind>{Type::INT, Type::LONG, Type::LONGLONG};
  else if (suffix == "u")
    candidates = {Type::UINT, Type::ULONG, Type::ULONGLONG};
  else if (suffix == "l")
    candidates = nondecimal
                     ? std::vector<Type::Kind>{Type::LONG, Type::ULONG,
                                               Type::LONGLONG, Type::ULONGLONG}
                     : std::vector<Type::Kind>{Type::LONG, Type::LONGLONG};
  else if (suffix == "ul" || suffix == "lu")
    candidates = {Type::ULONG, Type::ULONGLONG};
  else if (suffix == "ll")
    candidates = nondecimal
                     ? std::vector<Type::Kind>{Type::LONGLONG, Type::ULONGLONG}
                     : std::vector<Type::Kind>{Type::LONGLONG};
  else if (suffix == "ull" || suffix == "llu")
    candidates = {Type::ULONGLONG};
  else
    return fail(e->loc, "invalid integer literal suffix");
  for (auto kind : candidates) {
    Type *t = types.make(kind);
    int bytes = rank(t) == 3   ? target.sizeofInt
                : rank(t) == 4 ? target.sizeofLong
                               : target.sizeofLonglong;
    const auto maximum =
        bytes >= 8 ? static_cast<unsigned long long>(
                         std::numeric_limits<long long>::max())
                   : (1ULL << (bytes * 8 - (t->isUnsigned() ? 0 : 1))) - 1;
    if (e->val >= 0 && static_cast<unsigned long long>(e->val) <= maximum)
      return t;
  }
  return fail(e->loc, "integer literal is out of range for target");
}

Type *Sema::usualArithmetic(Type *a, Type *b) {
  for (auto k : {Type::LONGDOUBLE, Type::DOUBLE, Type::FLOAT})
    if (a->kind == k || b->kind == k)
      return types.make(k);
  a = promoted(a);
  b = promoted(b);
  if (a->kind == b->kind)
    return a;
  if (a->isUnsigned() == b->isUnsigned())
    return rank(a) >= rank(b) ? a : b;
  Type *u = a->isUnsigned() ? a : b;
  Type *s = a->isUnsigned() ? b : a;
  if (rank(u) >= rank(s))
    return u;
  auto width = [&](Type *t) {
    return rank(t) == 3   ? target.sizeofInt
           : rank(t) == 4 ? target.sizeofLong
                          : target.sizeofLonglong;
  };
  if (width(s) > width(u))
    return s;
  return types.make(s->kind == Type::LONG ? Type::ULONG : Type::ULONGLONG);
}

bool Sema::compatible(Type *a, Type *b, bool ignoreQuals) const {
  if (!a || !b || a->kind != b->kind ||
      (!ignoreQuals && !(a->quals == b->quals)))
    return false;
  switch (a->kind) {
  case Type::POINTER:
    return compatible(a->pointee, b->pointee);
  case Type::ARRAY:
    return (a->arraySize < 0 || b->arraySize < 0 ||
            a->arraySize == b->arraySize) &&
           compatible(a->elemType, b->elemType);
  case Type::FUNCTION:
    if (!compatible(a->retType, b->retType, true))
      return false;
    if (!a->hasPrototype || !b->hasPrototype) {
      Type *proto = a->hasPrototype ? a : b;
      if (!proto->hasPrototype)
        return true;
      if (proto->variadic)
        return false;
      for (auto *p : proto->params)
        if (p->kind == Type::FLOAT || (p->isIntegral() && rank(p) < 3))
          return false;
      return true;
    }
    if (a->variadic != b->variadic || a->params.size() != b->params.size())
      return false;
    for (size_t i = 0; i < a->params.size(); ++i)
      if (!compatible(a->params[i], b->params[i], true))
        return false;
    return true;
  case Type::STRUCT:
  case Type::UNION:
  case Type::ENUM:
    return a == b || (!a->tag.empty() && a->tag == b->tag);
  default:
    return true;
  }
}

Type *Sema::normalizeType(Type *t, SourceLoc loc) {
  if (!t)
    return nullptr;
  switch (t->kind) {
  case Type::TYPEDEF:
    return normalizeType(t->underlying, loc);
  case Type::STRUCT:
  case Type::UNION:
  case Type::ENUM: {
    if (t->tag.empty())
      return t;
    auto inserted = tags.emplace(t->tag, t);
    Type *canonical = inserted.first->second;
    if (canonical->kind != t->kind)
      fail(loc, "conflicting tag kind for '" + t->tag + "'");
    // Record bodies are not parsed yet. Reuse the tag's identity for
    // references.
    return canonical->quals == t->quals ? canonical
                                        : types.qualified(canonical, t->quals);
  }
  case Type::POINTER:
    t->pointee = normalizeType(t->pointee, loc);
    break;
  case Type::ARRAY:
    t->elemType = normalizeType(t->elemType, loc);
    break;
  case Type::FUNCTION:
    t->retType = normalizeType(t->retType, loc);
    for (auto *&p : t->params) {
      p = normalizeType(p, loc);
      // Check array bounds and element types before parameter adjustment.
      if (p && p->isArray())
        validateType(p, loc);
      if (p && p->isArray())
        p = types.ptrTo(p->elemType);
      else if (p && p->isFunction())
        p = types.ptrTo(p);
    }
    break;
  default:
    break;
  }
  return t;
}

bool Sema::validateType(Type *t, SourceLoc loc) {
  if (!t)
    return false;
  if (t->isPointer())
    return validateType(t->pointee, loc);
  if (t->isArray()) {
    if (!t->elemType || !t->elemType->isComplete()) {
      fail(loc, "array element type must be a complete object type");
      return false;
    }
    if (t->arraySize == 0) {
      fail(loc, "array size must be positive");
      return false;
    }
    return validateType(t->elemType, loc);
  }
  if (t->isFunction()) {
    if (!t->retType || t->retType->isArray() || t->retType->isFunction()) {
      fail(loc, "function cannot return an array or function type");
      return false;
    }
    if (!validateType(t->retType, loc))
      return false;
    if (t->variadic && t->params.empty()) {
      fail(loc, "variadic function requires at least one named parameter");
      return false;
    }
    for (auto *p : t->params) {
      if (p->isVoid()) {
        fail(loc, "parameter cannot have void type");
        return false;
      }
      if (!validateType(p, loc))
        return false;
    }
  }
  return true;
}

bool Sema::declare(Decl *d, bool global) {
  d->type = normalizeType(d->type, d->loc);
  if (!validateType(d->type, d->loc))
    return false;
  if (dynamic_cast<StructDecl *>(d) || dynamic_cast<UnionDecl *>(d) ||
      dynamic_cast<EnumDecl *>(d)) {
    d->canonicalDecl = d;
    return true; // Tags have a separate namespace.
  }
  if (auto *v = dynamic_cast<VarDecl *>(d)) {
    v->isGlobal = global;
    if (v->type->isVoid() || v->type->isFunction() ||
        (v->sc != StorageClass::EXTERN && !v->type->isComplete() &&
         !(global && v->type->isArray() && v->type->arraySize < 0))) {
      fail(v->loc,
           "variable '" + v->name + "' has incomplete or invalid object type");
      return false;
    }
    if (global &&
        (v->sc == StorageClass::AUTO || v->sc == StorageClass::REGISTER)) {
      fail(v->loc, "invalid storage class at file scope");
      return false;
    }
  }
  if (auto *f = dynamic_cast<FuncDecl *>(d)) {
    std::unordered_map<std::string, ParamDecl *> names;
    for (size_t i = 0; i < f->params.size(); ++i)
      f->params[i]->type = f->type->params[i];
    for (auto *p : f->params) {
      p->canonicalDecl = p;
      if (!p->name.empty() && !names.emplace(p->name, p).second) {
        fail(p->loc, "conflicting declaration of parameter '" + p->name + "'");
        diag.note(names[p->name]->loc, "previous parameter is here");
        return false;
      }
    }
    if (f->sc == StorageClass::AUTO || f->sc == StorageClass::REGISTER) {
      fail(f->loc, "invalid storage class for function");
      return false;
    }
  }
  Decl *previous = symbols.lookupCurrent(d->name);
  d->canonicalDecl = previous ? previous->canonicalDecl : d;
  if (previous) {
    const bool bothFunctions =
        dynamic_cast<FuncDecl *>(d) && dynamic_cast<FuncDecl *>(previous);
    const bool bothVars =
        dynamic_cast<VarDecl *>(d) && dynamic_cast<VarDecl *>(previous);
    const bool bothTypedefs =
        dynamic_cast<TypedefDecl *>(d) && dynamic_cast<TypedefDecl *>(previous);
    bool linkageConflict = false;
    if (bothVars) {
      auto oldSC = static_cast<VarDecl *>(previous)->sc;
      auto newSC = static_cast<VarDecl *>(d)->sc;
      linkageConflict =
          (newSC == StorageClass::STATIC && oldSC != StorageClass::STATIC) ||
          (oldSC == StorageClass::STATIC && newSC == StorageClass::NONE);
    }
    if (bothFunctions)
      linkageConflict =
          static_cast<FuncDecl *>(d)->sc == StorageClass::STATIC &&
          static_cast<FuncDecl *>(previous)->sc != StorageClass::STATIC;
    if ((!bothFunctions && !bothVars && !bothTypedefs) ||
        (!global && !bothTypedefs) || linkageConflict ||
        !compatible(previous->type, d->type)) {
      fail(d->loc, "conflicting declaration of '" + d->name + "'");
      diag.note(previous->loc, "previous declaration is here");
      return false;
    }
    if (d->type->isArray()) {
      if (d->type->arraySize < 0)
        d->type->arraySize = previous->type->arraySize;
      else if (previous->type->arraySize < 0)
        previous->type->arraySize = d->type->arraySize;
    }
  }
  if (auto *f = dynamic_cast<FuncDecl *>(d)) {
    auto defined = definitions.find(d->canonicalDecl);
    if ((f->body && !f->type->hasPrototype && previous &&
         previous->type->hasPrototype && !previous->type->params.empty()) ||
        (defined != definitions.end() && !defined->second->type->hasPrototype &&
         f->type->hasPrototype && !f->type->params.empty())) {
      fail(f->loc, "conflicting declaration of '" + f->name + "'");
      return false;
    }
    if (f->body && !definitions.emplace(d->canonicalDecl, f).second) {
      fail(f->loc, "redefinition of function '" + f->name + "'");
      diag.note(d->canonicalDecl->loc, "previous declaration is here");
      return false;
    }
  }
  if (!(previous && d->type->isFunction() && !d->type->hasPrototype &&
        previous->type->hasPrototype))
    symbols.bind(d);
  return true;
}

bool Sema::analyze(TranslationUnit &unit) {
  if (diag.hasErrors())
    return false;
  symbols.clear();
  symbols.push();
  tags.clear();
  definitions.clear();
  for (auto *d : unit.decls) {
    if (!declare(d, true))
      continue;
    if (auto *fn = dynamic_cast<FuncDecl *>(d)) {
      if (fn->body)
        analyzeFunction(fn);
    } else if (auto *v = dynamic_cast<VarDecl *>(d)) {
      if (v->init && analyzeExpr(v->init))
        convert(v->init, v->type, "initializer");
    }
  }
  // C completes an otherwise incomplete tentative array definition to one
  // element.
  for (auto *d : unit.decls)
    if (auto *v = dynamic_cast<VarDecl *>(d))
      if (v->sc != StorageClass::EXTERN && v->type->isArray() &&
          v->type->arraySize < 0)
        v->type->arraySize = 1;
  return !diag.hasErrors();
}

void Sema::analyzeFunction(FuncDecl *fn) {
  currentFunction = fn;
  symbols.push();
  if (fn->params.size() != fn->type->params.size())
    fail(fn->loc, "parameter names required in function definition");
  if (!fn->type->retType->isVoid() && !fn->type->retType->isComplete())
    fail(fn->loc, "function definition has incomplete return type");
  for (size_t i = 0; i < fn->params.size(); ++i) {
    auto *p = fn->params[i];
    p->type = fn->type->params[i];
    if (p->name.empty())
      fail(p->loc, "parameter name required in function definition");
    else if (!p->type->isComplete())
      fail(p->loc, "parameter '" + p->name + "' has incomplete type");
    else
      declare(p, false);
  }
  analyzeBlock(fn->body, false);
  symbols.pop();
  currentFunction = nullptr;
}

void Sema::analyzeBlock(CompoundStmt *block, bool nested) {
  if (nested)
    symbols.push();
  for (auto *item : block->items)
    analyzeNode(item);
  if (nested)
    symbols.pop();
}

void Sema::analyzeNode(Node *node) {
  if (auto *block = dynamic_cast<CompoundStmt *>(node))
    analyzeBlock(block);
  else if (auto *s = dynamic_cast<ExprStmt *>(node)) {
    if (s->expr && analyzeExpr(s->expr))
      value(s->expr);
  } else if (auto *s = dynamic_cast<ReturnStmt *>(node)) {
    if (!currentFunction) {
      fail(s->loc, "return outside function");
      return;
    }
    Type *ret = currentFunction->type->retType;
    if (s->value) {
      if (analyzeExpr(s->value)) {
        if (ret->isVoid())
          fail(s->loc, "void function cannot return a value");
        else
          convert(s->value, ret, "return");
      }
    } else if (!ret->isVoid())
      fail(s->loc, "non-void function must return a value");
  } else if (auto *d = dynamic_cast<Decl *>(node)) {
    if (declare(d, false))
      if (auto *v = dynamic_cast<VarDecl *>(d))
        if (v->init && analyzeExpr(v->init))
          convert(v->init, v->type, "initializer");
  } else
    fail(node->loc, "unsupported statement in semantic analysis");
}

void Sema::cast(Expr *&expr, Type *to, ImplicitCastKind kind) {
  auto c = std::make_unique<ImplicitCastExpr>();
  c->loc = expr->loc;
  c->operand = expr;
  c->type = to;
  c->kind = kind;
  expr = c.get();
  casts.push_back(std::move(c));
}

Type *Sema::value(Expr *&expr) {
  if (!expr || !expr->type)
    return nullptr;
  Type *t = expr->type;
  if (t->isArray())
    cast(expr, types.ptrTo(t->elemType), ImplicitCastKind::ArrayToPointer);
  else if (t->isFunction())
    cast(expr, types.ptrTo(t), ImplicitCastKind::FunctionToPointer);
  else if (expr->isLval)
    cast(expr, unqualified(t), ImplicitCastKind::LValueToRValue);
  return expr->type;
}

bool Sema::nullConstant(const Expr *e) const {
  if (auto *c = dynamic_cast<const ImplicitCastExpr *>(e))
    return c->type->isIntegral() && nullConstant(c->operand);
  if (auto *lit = dynamic_cast<const IntLitExpr *>(e))
    return lit->val == 0;
  if (auto *u = dynamic_cast<const UnaryExpr *>(e))
    return (u->op == UnaryOp::POS || u->op == UnaryOp::NEG) &&
           nullConstant(u->operand);
  return false;
}

bool Sema::pointerConvertible(Type *to, Type *from) const {
  if (!to->isPointer() || !from->isPointer())
    return false;
  Type *a = to->pointee, *b = from->pointee;
  if ((b->quals.isConst && !a->quals.isConst) ||
      (b->quals.isVolatile && !a->quals.isVolatile))
    return false;
  return compatible(a, b, true) || (a->isVoid() && !b->isFunction()) ||
         (b->isVoid() && !a->isFunction());
}

Type *Sema::commonPointer(Type *a, Type *b, bool allowVoid) {
  Type *pa = a->pointee, *pb = b->pointee;
  if (!compatible(pa, pb, true) &&
      !(allowVoid && ((pa->isVoid() && !pb->isFunction()) ||
                      (pb->isVoid() && !pa->isFunction()))))
    return nullptr;
  Type *p = pa->isVoid() ? pa : pb->isVoid() ? pb : pa;
  Qualifiers quals{pa->quals.isConst || pb->quals.isConst,
                   pa->quals.isVolatile || pb->quals.isVolatile};
  return types.ptrTo(types.qualified(p, quals));
}

bool Sema::convert(Expr *&expr, Type *to, const std::string &context) {
  Type *from = value(expr);
  if (!from)
    return false;
  to = unqualified(to);
  if (compatible(to, from, true))
    return true;
  if (to->isArithmetic() && from->isArithmetic())
    cast(expr, to, ImplicitCastKind::Arithmetic);
  else if (to->isPointer() && nullConstant(expr))
    cast(expr, to, ImplicitCastKind::NullToPointer);
  else if (pointerConvertible(to, from))
    cast(expr, to, ImplicitCastKind::Pointer);
  else {
    fail(expr->loc, "incompatible " + context + " type: cannot convert '" +
                        from->str() + "' to '" + to->str() + "'");
    return false;
  }
  return true;
}

void Sema::defaultPromote(Expr *&expr) {
  Type *t = value(expr);
  if (!t)
    return;
  Type *to = t->kind == Type::FLOAT ? types.doubleTy() : promoted(t);
  if (!compatible(t, to))
    cast(expr, to, ImplicitCastKind::DefaultArgumentPromotion);
}

bool Sema::modifiable(const Expr *e) const {
  return e && e->type && e->isLval && !e->type->quals.isConst &&
         !e->type->isArray() && e->type->isComplete();
}
bool Sema::objectPointer(Type *t) const {
  return t && t->isPointer() && t->pointee && t->pointee->isComplete();
}

Type *Sema::analyzeUnary(UnaryExpr *e) {
  Type *t = analyzeExpr(e->operand);
  if (!t)
    return nullptr;
  if (e->op == UnaryOp::ADDR) {
    if (!e->operand->isLval && !t->isFunction())
      return fail(e->loc, "address-of requires an lvalue or function");
    if (auto *id = dynamic_cast<IdentExpr *>(e->operand))
      if (auto *v = dynamic_cast<VarDecl *>(id->decl))
        if (v->sc == StorageClass::REGISTER)
          return fail(e->loc, "cannot take address of register variable");
    return types.ptrTo(t);
  }
  if (e->op == UnaryOp::PRE_INC || e->op == UnaryOp::PRE_DEC ||
      e->op == UnaryOp::POST_INC || e->op == UnaryOp::POST_DEC) {
    if (!modifiable(e->operand) || !(t->isArithmetic() || objectPointer(t)))
      return fail(e->loc, "increment/decrement requires a modifiable "
                          "arithmetic or object-pointer lvalue");
    return unqualified(t);
  }
  t = value(e->operand);
  if (e->op == UnaryOp::DEREF) {
    if (!t->isPointer() || t->pointee->isVoid())
      return fail(e->loc,
                  "dereference requires a pointer to object or function");
    e->isLval = !t->pointee->isFunction();
    return t->pointee;
  }
  if (e->op == UnaryOp::NOT) {
    if (!t->isScalar())
      return fail(e->loc, "logical not requires a scalar operand");
    return types.intTy();
  }
  if (!t->isArithmetic() || (e->op == UnaryOp::BITNOT && !t->isIntegral()))
    return fail(e->loc, "invalid operand type for unary operator");
  Type *to = promoted(t);
  if (!compatible(to, t))
    cast(e->operand, to, ImplicitCastKind::Arithmetic);
  return to;
}

Type *Sema::binaryType(BinaryOp op, Expr *&lhs, Expr *&rhs, SourceLoc loc) {
  Type *a = value(lhs), *b = value(rhs);
  if (!a || !b)
    return nullptr;
  auto arithmetic = [&]() -> Type * {
    Type *common = usualArithmetic(a, b);
    convert(lhs, common, "operand");
    convert(rhs, common, "operand");
    return common;
  };
  if (op == BinaryOp::COMMA)
    return b;
  if (op == BinaryOp::AND || op == BinaryOp::OR) {
    if (a->isScalar() && b->isScalar())
      return types.intTy();
  } else if (op >= BinaryOp::EQ && op <= BinaryOp::GEQ) {
    const bool equality = op == BinaryOp::EQ || op == BinaryOp::NEQ;
    if (a->isArithmetic() && b->isArithmetic()) {
      arithmetic();
      return types.intTy();
    }
    if (a->isPointer() && b->isPointer() &&
        (equality || (!a->pointee->isFunction() && !a->pointee->isVoid()))) {
      if (Type *common = commonPointer(a, b, equality)) {
        convert(lhs, common, "comparison");
        convert(rhs, common, "comparison");
        return types.intTy();
      }
    }
    if (equality && a->isPointer() && nullConstant(rhs)) {
      convert(rhs, a, "comparison");
      return types.intTy();
    }
    if (equality && b->isPointer() && nullConstant(lhs)) {
      convert(lhs, b, "comparison");
      return types.intTy();
    }
  } else if (op == BinaryOp::ADD || op == BinaryOp::SUB) {
    if (a->isArithmetic() && b->isArithmetic())
      return arithmetic();
    if (objectPointer(a) && b->isIntegral()) {
      convert(rhs, promoted(b), "offset");
      return a;
    }
    if (op == BinaryOp::ADD && a->isIntegral() && objectPointer(b)) {
      convert(lhs, promoted(a), "offset");
      return b;
    }
    if (op == BinaryOp::SUB && objectPointer(a) && objectPointer(b) &&
        compatible(a->pointee, b->pointee, true))
      return sizeType(true);
  } else if (op == BinaryOp::LSHIFT || op == BinaryOp::RSHIFT) {
    if (a->isIntegral() && b->isIntegral()) {
      Type *result = promoted(a);
      convert(lhs, result, "operand");
      convert(rhs, promoted(b), "operand");
      return result;
    }
  } else {
    const bool integralOnly = op == BinaryOp::MOD || op == BinaryOp::BITAND ||
                              op == BinaryOp::BITOR || op == BinaryOp::BITXOR;
    if (a->isArithmetic() && b->isArithmetic() &&
        (!integralOnly || (a->isIntegral() && b->isIntegral())))
      return arithmetic();
  }
  return fail(loc, "invalid operands to binary operator ('" + a->str() +
                       "' and '" + b->str() + "')");
}

Type *Sema::analyzeBinary(BinaryExpr *e) {
  Type *a = analyzeExpr(e->lhs);
  Type *b = analyzeExpr(e->rhs);
  if (!a || !b)
    return nullptr;
  if (!assignment(e->op))
    return binaryType(e->op, e->lhs, e->rhs, e->loc);
  if (!modifiable(e->lhs))
    return fail(e->loc, "assignment requires a modifiable lvalue");
  if (e->op == BinaryOp::ASSIGN) {
    if (!convert(e->rhs, a, "assignment"))
      return nullptr;
    e->computationType = unqualified(a);
  } else {
    // Analyze the loaded lhs through a separate edge. Keep its address in the
    // AST so the backend evaluates a compound assignment's lhs exactly once.
    Expr *loaded = e->lhs;
    e->computationType =
        binaryType(compoundOperation(e->op), loaded, e->rhs, e->loc);
    if (!e->computationType)
      return nullptr;
    if (!(a->isArithmetic() && e->computationType->isArithmetic()) &&
        !compatible(a, e->computationType, true))
      return fail(e->loc, "incompatible compound assignment result");
  }
  return unqualified(a);
}

Type *Sema::analyzeConditional(TernaryExpr *e) {
  Type *cond = analyzeExpr(e->cond);
  Type *a = analyzeExpr(e->then), *b = analyzeExpr(e->els);
  if (!cond || !a || !b)
    return nullptr;
  if (!value(e->cond)->isScalar())
    return fail(e->loc, "conditional requires a scalar condition");
  a = value(e->then);
  b = value(e->els);
  Type *common = nullptr;
  if (a->isArithmetic() && b->isArithmetic())
    common = usualArithmetic(a, b);
  else if (a->isVoid() && b->isVoid())
    common = a;
  else if (a->isPointer() && nullConstant(e->els))
    common = a;
  else if (b->isPointer() && nullConstant(e->then))
    common = b;
  else if (a->isPointer() && b->isPointer()) {
    common = commonPointer(a, b, true);
  } else if (compatible(a, b, true))
    common = unqualified(a);
  if (!common)
    return fail(e->loc, "incompatible conditional operands");
  convert(e->then, common, "conditional");
  convert(e->els, common, "conditional");
  return common;
}

Type *Sema::analyzeExpr(Expr *&expr) {
  if (!expr)
    return nullptr;
  if (dynamic_cast<ImplicitCastExpr *>(expr))
    return expr->type;
  expr->isLval = false;
  Type *result = nullptr;
  if (auto *lit = dynamic_cast<IntLitExpr *>(expr))
    result = integerLiteralType(lit);
  else if (dynamic_cast<CharLitExpr *>(expr))
    result = types.intTy();
  else if (auto *lit = dynamic_cast<FloatLitExpr *>(expr)) {
    const auto lastNumeric = lit->spelling.find_last_not_of("uUlLfF");
    std::string suffix = lastNumeric == std::string::npos
                             ? ""
                             : lit->spelling.substr(lastNumeric + 1);
    if (suffix.empty())
      result = types.doubleTy();
    else if (suffix == "f" || suffix == "F")
      result = types.floatTy();
    else if (suffix == "l" || suffix == "L")
      result = types.make(Type::LONGDOUBLE);
    else
      return fail(lit->loc, "invalid floating literal suffix");
  } else if (auto *lit = dynamic_cast<StringLitExpr *>(expr)) {
    result =
        types.arrayOf(types.charTy(), static_cast<int>(lit->val.size() + 1));
    expr->isLval = true;
  } else if (auto *id = dynamic_cast<IdentExpr *>(expr)) {
    id->decl = symbols.lookup(id->name);
    if (!id->decl || dynamic_cast<TypedefDecl *>(id->decl))
      return fail(id->loc, "use of undeclared identifier '" + id->name + "'");
    result = id->decl->type;
    expr->isLval = !result->isFunction();
  } else if (auto *u = dynamic_cast<UnaryExpr *>(expr))
    result = analyzeUnary(u);
  else if (auto *b = dynamic_cast<BinaryExpr *>(expr))
    result = analyzeBinary(b);
  else if (auto *t = dynamic_cast<TernaryExpr *>(expr))
    result = analyzeConditional(t);
  else if (auto *c = dynamic_cast<CallExpr *>(expr)) {
    Type *callee = analyzeExpr(c->callee);
    bool argsOK = true;
    for (auto *&arg : c->args)
      if (!analyzeExpr(arg))
        argsOK = false;
    if (!callee || !argsOK)
      return nullptr;
    callee = value(c->callee);
    if (!callee->isPointer() || !callee->pointee->isFunction())
      return fail(c->loc,
                  "called expression is not a function or function pointer");
    Type *fn = callee->pointee;
    if (fn->hasPrototype &&
        (c->args.size() < fn->params.size() ||
         (!fn->variadic && c->args.size() != fn->params.size())))
      return fail(c->loc, "incorrect number of arguments in function call");
    for (size_t i = 0; i < c->args.size(); ++i) {
      if (fn->hasPrototype && i < fn->params.size()) {
        if (!convert(c->args[i], fn->params[i], "argument"))
          return nullptr;
      } else
        defaultPromote(c->args[i]);
    }
    if (!fn->retType->isVoid() && !fn->retType->isComplete())
      return fail(c->loc, "call returns an incomplete type");
    result = unqualified(fn->retType);
  } else if (auto *idx = dynamic_cast<IndexExpr *>(expr)) {
    Type *a = analyzeExpr(idx->base), *b = analyzeExpr(idx->index);
    if (!a || !b)
      return nullptr;
    a = value(idx->base);
    b = value(idx->index);
    Expr **integer = &idx->index;
    if (a->isIntegral() && b->isPointer()) {
      std::swap(a, b);
      integer = &idx->base;
    }
    if (!objectPointer(a) || !b->isIntegral())
      return fail(idx->loc,
                  "indexing requires an object pointer and an integer");
    convert(*integer, promoted(b), "index");
    result = a->pointee;
    expr->isLval = true;
  } else if (auto *m = dynamic_cast<MemberExpr *>(expr)) {
    Type *base = analyzeExpr(m->base);
    if (!base)
      return nullptr;
    if (m->arrow) {
      base = value(m->base);
      if (!base->isPointer())
        return fail(m->loc, "arrow member access requires a pointer");
      base = base->pointee;
    }
    if (!(base->isStruct() || base->isUnion()) || !base->isComplete())
      return fail(m->loc, "member access requires a complete struct or union");
    auto field =
        std::find_if(base->fields.begin(), base->fields.end(),
                     [&](const Type::Field &f) { return f.name == m->member; });
    if (field == base->fields.end())
      return fail(m->loc, "no member named '" + m->member + "'");
    Qualifiers q{base->quals.isConst || field->type->quals.isConst,
                 base->quals.isVolatile || field->type->quals.isVolatile};
    result = types.qualified(field->type, q);
    expr->isLval = m->arrow || m->base->isLval;
  } else if (auto *s = dynamic_cast<SizeofExpr *>(expr)) {
    Type *operand = s->ofType ? normalizeType(s->operandType, s->loc)
                              : analyzeExpr(s->expr);
    s->operandType = operand;
    if (!operand)
      return nullptr;
    if (!validateType(operand, s->loc))
      return nullptr;
    if (!operand->isComplete())
      return fail(s->loc, "sizeof requires a complete object type");
    result = sizeType(); // Do not decay or load the unevaluated operand.
  } else if (auto *c = dynamic_cast<CastExpr *>(expr)) {
    Type *operand = analyzeExpr(c->operand);
    c->toType = normalizeType(c->toType, c->loc);
    if (!operand || !validateType(c->toType, c->loc))
      return nullptr;
    operand = value(c->operand);
    if (!c->toType->isVoid() &&
        (!c->toType->isScalar() || !operand->isScalar() ||
         (c->toType->isPointer() && operand->isFloating()) ||
         (c->toType->isFloating() && operand->isPointer())))
      return fail(c->loc, "invalid cast operands");
    result = unqualified(c->toType);
  } else
    return fail(expr->loc, "unsupported expression in semantic analysis");
  expr->type = result;
  return result;
}
