#include "SemaInternal.hpp"
#include "cc1/Sema/Sema.hpp"
#include <cctype>
#include <limits>

using cc1::sema_detail::rank;

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

void Sema::cast(Expr *&expr, Type *to, ImplicitCastKind kind) {
  auto *c = ast.create<ImplicitCastExpr>();
  c->loc = expr->loc;
  c->operand = expr;
  c->type = to;
  c->kind = kind;
  expr = c;
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
