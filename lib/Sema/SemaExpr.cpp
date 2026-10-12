#include "SemaInternal.hpp"
#include "cc1/Sema/Sema.hpp"
#include <algorithm>
#include <cctype>

using cc1::sema_detail::assignment;
using cc1::sema_detail::compoundOperation;

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
    result = sizeType();
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
