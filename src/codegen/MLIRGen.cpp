#include "MLIRGen.hpp"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlowOps.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/IR/Verifier.h"
#include "llvm/ADT/SmallVector.h"

MLIRGen::MLIRGen(mlir::MLIRContext &context, DiagnosticEngine &diagnostics,
                 const TargetInfo &targetInfo)
    : builder(&context), diag(diagnostics), target(targetInfo) {
  context
      .loadDialect<mlir::arith::ArithDialect, mlir::func::FuncDialect,
                   mlir::memref::MemRefDialect, mlir::cf::ControlFlowDialect>();
}

mlir::OwningOpRef<mlir::ModuleOp>
MLIRGen::generate(const TranslationUnit &unit) {
  mlir::OwningOpRef<mlir::ModuleOp> module(
      mlir::ModuleOp::create(builder.getUnknownLoc()));

  functions.clear();
  values.clear();
  storage.clear();
  llvm::DenseMap<const Decl *, const FuncDecl *> selected;
  llvm::SmallVector<const Decl *> order;
  for (const Decl *decl : unit.decls) {
    if (dynamic_cast<const TypedefDecl *>(decl))
      continue;
    auto *function = dynamic_cast<const FuncDecl *>(decl);
    if (!function) {
      diag.error(decl->loc, "MLIR generation is not implemented "
                            "for this declaration yet");
      return {};
    }
    const Decl *key =
        function->canonicalDecl ? function->canonicalDecl : function;
    auto found = selected.find(key);
    if (found == selected.end()) {
      order.push_back(key);
      selected[key] = function;
    } else if (function->body ||
               (!found->second->body && function->type->hasPrototype)) {
      selected[key] = function;
    }
  }
  for (const Decl *key : order) {
    builder.setInsertionPointToEnd(module->getBody());
    if (mlir::failed(declareFunction(*selected[key])))
      return {};
  }
  for (const Decl *key : order) {
    const FuncDecl &function = *selected[key];
    if (function.body && mlir::failed(emitFunction(function)))
      return {};
  }

  if (mlir::failed(mlir::verify(module.get()))) {
    diag.error(unit.loc, "generated MLIR module is invalid");
    return {};
  }

  return module;
}
