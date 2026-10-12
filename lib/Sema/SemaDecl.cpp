#include "SemaInternal.hpp"
#include "cc1/Sema/Sema.hpp"

using cc1::sema_detail::rank;

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
