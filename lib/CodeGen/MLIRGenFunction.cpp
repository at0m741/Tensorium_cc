#include "cc1/CodeGen/MLIRGen.hpp"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "llvm/ADT/SmallVector.h"

mlir::LogicalResult MLIRGen::emitFunction(const FuncDecl &function) {
  const Decl *key = function.canonicalDecl ? function.canonicalDecl : &function;
  auto op = functions.lookup(key);
  mlir::OpBuilder::InsertionGuard guard(builder);
  mlir::Block *entry = op.addEntryBlock();
  builder.setInsertionPointToStart(entry);
  if (function.sc != StorageClass::STATIC &&
      static_cast<const FuncDecl *>(key)->sc != StorageClass::STATIC)
    op.setPublic();
  values.clear();
  storage.clear();
  loopTargets.clear();
  if (function.params.size() != entry->getNumArguments()) {
    diag.error(function.loc, "MLIR parameter count mismatch");
    return mlir::failure();
  }
  for (unsigned i = 0; i < function.params.size(); ++i)
    values[function.params[i]] = entry->getArgument(i);
  if (mlir::failed(emitBlock(*function.body, op))) {
    values.clear();
    storage.clear();
    return mlir::failure();
  }
  auto *exitBlock = builder.getInsertionBlock();

  if (exitBlock->empty() ||
      !exitBlock->back().hasTrait<mlir::OpTrait::IsTerminator>()) {
    llvm::SmallVector<mlir::Value> results;
    if (function.type->retType->isVoid()) {
    } else if (function.name == "main") {
      results.push_back(
          mlir::arith::ConstantIntOp::create(builder, builder.getUnknownLoc(),
                                             op.getResultTypes().front(), 0)
              .getResult());
    } else {
      diag.error(function.loc,
                 "MLIR generation requires a return in a non-void function");
      values.clear();
      storage.clear();
      return mlir::failure();
    }
    mlir::func::ReturnOp::create(builder, builder.getUnknownLoc(), results);
  }
  values.clear();
  storage.clear();
  return mlir::success();
}

mlir::LogicalResult MLIRGen::emitCall(const CallExpr &call,
                                      mlir::Value *result) {
  const Expr *callee = call.callee;
  if (auto *cast = dynamic_cast<const ImplicitCastExpr *>(callee))
    if (cast->kind == ImplicitCastKind::FunctionToPointer)
      callee = cast->operand;
  auto *ref = dynamic_cast<const IdentExpr *>(callee);
  auto *decl = ref ? dynamic_cast<const FuncDecl *>(ref->decl) : nullptr;
  const Decl *key = decl && decl->canonicalDecl ? decl->canonicalDecl : decl;
  auto found = functions.find(key);
  if (!decl || found == functions.end()) {
    diag.error(call.loc,
               "MLIR generation currently supports only direct function calls");
    return mlir::failure();
  }
  auto function = found->second;
  if (call.args.size() != function.getNumArguments()) {
    diag.error(
        call.loc,
        "MLIR call argument count does not match the function signature");
    return mlir::failure();
  }
  llvm::SmallVector<mlir::Value> args;
  for (size_t i = 0; i < call.args.size(); ++i) {
    mlir::Value value = emitExpr(*call.args[i]);
    if (!value)
      return mlir::failure();
    if (value.getType() != function.getArgumentTypes()[i]) {
      diag.error(
          call.args[i]->loc,
          "MLIR call argument type does not match the function signature");
      return mlir::failure();
    }
    args.push_back(value);
  }
  auto op = mlir::func::CallOp::create(builder, builder.getUnknownLoc(),
                                       function, args);
  if (result)
    *result = op.getNumResults() ? op.getResult(0) : mlir::Value{};
  return mlir::success();
}
