#include "cc1/Sema/Sema.hpp"

Type *Sema::fail(SourceLoc loc, const std::string &message) {
  diag.error(loc, message);
  return nullptr;
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
  loopDepth = 0;
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
