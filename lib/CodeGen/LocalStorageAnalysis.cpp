#include "cc1/CodeGen/LocalStorageAnalysis.hpp"
#include "cc1/AST/AST.hpp"
#include "cc1/AST/Type.hpp"

void LocalStorageAnalysis::analyze(const FuncDecl &function) {
  usage.clear();
  conservative = false;
  initializingDecl = nullptr;

  for (const auto *parameter : function.params)
    usage.emplace(parameter, Usage{});

  if (!function.body) {
    conservative = true;
    return;
  }
  visitNode(function.body);
}

bool LocalStorageAnalysis::needsStorage(const Decl *decl) const {
  if (conservative || !decl || !decl->type)
    return true;
  auto found = usage.find(decl);
  if (found == usage.end())
    return true;
  if (!decl->type->isScalar() || decl->type->quals.isVolatile)
    return true;
  const Usage &info = found->second;
  return info.written || info.addressTaken || info.referencedInInitializer;
}

void LocalStorageAnalysis::visitNode(const Node *node) {
  if (!node)
    return;

  if (auto *expr = dynamic_cast<const Expr *>(node)) {
    visitExpr(expr);
  } else if (auto *block = dynamic_cast<const CompoundStmt *>(node)) {
    for (const auto *item : block->items)
      visitNode(item);
  } else if (auto *var = dynamic_cast<const VarDecl *>(node)) {
    const bool automatic = var->sc == StorageClass::NONE ||
                           var->sc == StorageClass::AUTO ||
                           var->sc == StorageClass::REGISTER;

    if (!var->isGlobal && automatic && var->init)
      usage.emplace(var, Usage{});

    const Decl *previous = initializingDecl;
    initializingDecl = var;
    visitExpr(var->init);
    initializingDecl = previous;
  } else if (auto *stmt = dynamic_cast<const ExprStmt *>(node)) {
    visitExpr(stmt->expr);
  } else if (auto *stmt = dynamic_cast<const ReturnStmt *>(node)) {
    visitExpr(stmt->value);
  } else if (auto *stmt = dynamic_cast<const IfStmt *>(node)) {
    visitExpr(stmt->cond);
    visitNode(stmt->then);
    visitNode(stmt->els);
  } else if (auto *stmt = dynamic_cast<const WhileStmt *>(node)) {
    visitExpr(stmt->cond);
    visitNode(stmt->body);
  } else if (auto *stmt = dynamic_cast<const ForStmt *>(node)) {
    visitNode(stmt->init);
    visitExpr(stmt->cond);
    visitExpr(stmt->incr);
    visitNode(stmt->body);
  } else if (!dynamic_cast<const BreakStmt *>(node) &&
             !dynamic_cast<const ContinueStmt *>(node) &&
             !dynamic_cast<const TypedefDecl *>(node)) {
    // A new or unsupported node may contain writes or address-taking.
    conservative = true;
  }
}

void LocalStorageAnalysis::visitExpr(const Expr *expr) {
  if (!expr)
    return;

  if (auto *reference = dynamic_cast<const IdentExpr *>(expr)) {
    if (reference->decl == initializingDecl) {
      auto found = usage.find(reference->decl);
      if (found != usage.end())
        found->second.referencedInInitializer = true;
    }
    return;
  }

  if (dynamic_cast<const IntLitExpr *>(expr) ||
      dynamic_cast<const FloatLitExpr *>(expr) ||
      dynamic_cast<const CharLitExpr *>(expr) ||
      dynamic_cast<const StringLitExpr *>(expr))
    return;
  if (dynamic_cast<const SizeofExpr *>(expr)) {
    // The frontend only supports fixed-size types, so sizeof never evaluates
    // its operand. Revisit this when variably modified types are supported.
    return;
  }

  if (auto *cast = dynamic_cast<const ImplicitCastExpr *>(expr)) {
    visitExpr(cast->operand);
  } else if (auto *cast = dynamic_cast<const CastExpr *>(expr)) {
    visitExpr(cast->operand);
  } else if (auto *unary = dynamic_cast<const UnaryExpr *>(expr)) {
    switch (unary->op) {
    case UnaryOp::ADDR:
      markAddressTaken(unary->operand);
      break;
    case UnaryOp::PRE_INC:
    case UnaryOp::PRE_DEC:
    case UnaryOp::POST_INC:
    case UnaryOp::POST_DEC:
      markWritten(unary->operand);
      break;
    case UnaryOp::NEG:
    case UnaryOp::NOT:
    case UnaryOp::BITNOT:
    case UnaryOp::DEREF:
    case UnaryOp::POS:
      break;
    default:
      conservative = true;
      break;
    }
    visitExpr(unary->operand);
  } else if (auto *binary = dynamic_cast<const BinaryExpr *>(expr)) {
    if (binary->op >= BinaryOp::ASSIGN && binary->op <= BinaryOp::RSHIFT_ASSIGN)
      markWritten(binary->lhs);
    visitExpr(binary->lhs);
    visitExpr(binary->rhs);
  } else if (auto *conditional = dynamic_cast<const TernaryExpr *>(expr)) {
    visitExpr(conditional->cond);
    visitExpr(conditional->then);
    visitExpr(conditional->els);
  } else if (auto *call = dynamic_cast<const CallExpr *>(expr)) {
    visitExpr(call->callee);
    for (const auto *argument : call->args)
      visitExpr(argument);
  } else if (auto *index = dynamic_cast<const IndexExpr *>(expr)) {
    visitExpr(index->base);
    visitExpr(index->index);
  } else if (auto *member = dynamic_cast<const MemberExpr *>(expr)) {
    visitExpr(member->base);
  } else {
    conservative = true;
  }
}

void LocalStorageAnalysis::markWritten(const Expr *expr) {
  if (auto *reference = dynamic_cast<const IdentExpr *>(expr)) {
    auto found = usage.find(reference->decl);
    if (found != usage.end())
      found->second.written = true;
  } else if (auto *unary = dynamic_cast<const UnaryExpr *>(expr)) {
    if (unary->op != UnaryOp::DEREF)
      conservative = true;
  } else if (auto *member = dynamic_cast<const MemberExpr *>(expr)) {
    if (!member->arrow)
      markWritten(member->base);
  } else if (!dynamic_cast<const IndexExpr *>(expr)) {
    conservative = true;
  }
  // Writing through a pointer does not modify the pointer object. visitExpr
  // still finds updates within its address expression, such as *p++ = value.
}

void LocalStorageAnalysis::markAddressTaken(const Expr *expr) {
  if (auto *reference = dynamic_cast<const IdentExpr *>(expr)) {
    auto found = usage.find(reference->decl);
    if (found != usage.end())
      found->second.addressTaken = true;
  } else if (auto *unary = dynamic_cast<const UnaryExpr *>(expr)) {
    if (unary->op != UnaryOp::DEREF)
      conservative = true;
  } else if (auto *member = dynamic_cast<const MemberExpr *>(expr)) {
    if (!member->arrow)
      markAddressTaken(member->base);
  } else if (!dynamic_cast<const IndexExpr *>(expr)) {
    conservative = true;
  }
}
