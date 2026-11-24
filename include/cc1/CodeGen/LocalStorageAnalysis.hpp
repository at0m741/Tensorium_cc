#pragma once

#include <unordered_map>

struct Decl;
struct Expr;
struct FuncDecl;
struct Node;

class LocalStorageAnalysis {
public:
  // Analyze a function after semantic analysis.
  void analyze(const FuncDecl &function);
  // Only read-only scalar parameters without an observable address can use SSA.
  // Locals and unknown declarations conservatively require memory storage.
  bool needsStorage(const Decl *decl) const;

private:
  struct Usage {
    bool written = false;
    bool addressTaken = false;
    bool referencedInInitializer = false;
  };

  std::unordered_map<const Decl *, Usage> usage;

  const Decl *initializingDecl = nullptr;
  bool conservative = false;

  void visitNode(const Node *node);
  void visitExpr(const Expr *expr);

  void markWritten(const Expr *expr);
  void markAddressTaken(const Expr *expr);
};
