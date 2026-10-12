#include "cc1/Sema/Sema.hpp"

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

  } else if (auto *stmt = dynamic_cast<IfStmt *>(node)) {
    Type *condition = analyzeExpr(stmt->cond);

    if (condition) {
      condition = value(stmt->cond);

      if (!condition->isScalar())
        fail(stmt->cond->loc, "if requieres a scalar operand");
    }

    if (stmt->then)
      analyzeNode(stmt->then);
    if (stmt->els)
      analyzeNode(stmt->els);

  } else if (auto *stmt = dynamic_cast<BreakStmt *>(node)) {
    if (loopDepth == 0)
      fail(stmt->loc, "break outside loop");

  } else if (auto *stmt = dynamic_cast<ContinueStmt *>(node)) {
    if (loopDepth == 0)
      fail(stmt->loc, "continue outside loop");
  } else if (auto *stmt = dynamic_cast<WhileStmt *>(node)) {
    Type *condition = analyzeExpr(stmt->cond);

    if (condition) {
      condition = value(stmt->cond);

      if (!condition->isScalar())
        fail(stmt->cond->loc, "while requieres a scalar condition");
    }

    ++loopDepth;

    if (stmt->body)
      analyzeNode(stmt->body);

    --loopDepth;
  } else if (auto *stmt = dynamic_cast<ForStmt *>(node)) {
    symbols.push();

    if (auto *decl = dynamic_cast<Decl *>(stmt->init)) {
      auto *var = dynamic_cast<VarDecl *>(decl);
      if (!var ||
          (var->sc != StorageClass::NONE && var->sc != StorageClass::AUTO &&
           var->sc != StorageClass::REGISTER))
        fail(decl->loc, "for initializer declaration requires an automatic or "
                        "register object");
      else
        analyzeNode(var);
    } else if (stmt->init) {
      analyzeNode(stmt->init);
    }

    if (stmt->cond && analyzeExpr(stmt->cond)) {
      Type *condition = value(stmt->cond);
      if (!condition->isScalar())
        fail(stmt->cond->loc, "for requires a scalar condition");
    }

    if (stmt->incr && analyzeExpr(stmt->incr))
      value(stmt->incr);

    ++loopDepth;
    if (stmt->body)
      analyzeNode(stmt->body);
    --loopDepth;

    symbols.pop();

  } else if (auto *d = dynamic_cast<Decl *>(node)) {
    if (declare(d, false))
      if (auto *v = dynamic_cast<VarDecl *>(d))
        if (v->init && analyzeExpr(v->init))
          convert(v->init, v->type, "initializer");
  } else
    fail(node->loc, "unsupported statement in semantic analysis");
}
