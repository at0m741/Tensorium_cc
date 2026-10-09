#include "MLIRGen.hpp"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "llvm/ADT/SmallVector.h"

mlir::LogicalResult MLIRGen::declareFunction(const FuncDecl &function) {
  if (function.type->variadic || (function.type->retType->kind != Type::INT &&
                                  !function.type->retType->isVoid())) {
    diag.error(function.loc, "MLIR generation is not implemented "
                             "for this declaration yet");
    return mlir::failure();
  }
  llvm::SmallVector<mlir::Type> params, results;
  for (const Type *param : function.type->params) {
    if (param->quals.isVolatile) {
      diag.error(
          function.loc,
          "MLIR generation does not support volatile parameter storage yet");
      return mlir::failure();
    }
    mlir::Type type = lowerType(*param, function.loc);
    if (!type)
      return mlir::failure();
    params.push_back(type);
  }
  if (!function.type->retType->isVoid()) {
    mlir::Type type = lowerType(*function.type->retType, function.loc);
    if (!type)
      return mlir::failure();
    results.push_back(type);
  }
  auto signature = builder.getFunctionType(params, results);
  auto op = builder.create<mlir::func::FuncOp>(builder.getUnknownLoc(),
                                               function.name, signature);
  op.setPrivate();
  const Decl *key = function.canonicalDecl ? function.canonicalDecl : &function;
  functions[key] = op;
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
  auto op = builder.create<mlir::func::CallOp>(builder.getUnknownLoc(),
                                               function, args);
  if (result)
    *result = op.getNumResults() ? op.getResult(0) : mlir::Value{};
  return mlir::success();
}

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
          builder
              .create<mlir::arith::ConstantIntOp>(builder.getUnknownLoc(), 0,
                                                  op.getResultTypes().front())
              .getResult());
    } else {
      diag.error(function.loc,
                 "MLIR generation requires a return in a non-void function");
      values.clear();
      storage.clear();
      return mlir::failure();
    }
    builder.create<mlir::func::ReturnOp>(builder.getUnknownLoc(), results);
  }
  values.clear();
  storage.clear();
  return mlir::success();
}
