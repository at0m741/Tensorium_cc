#include "MLIRGen.hpp"

#include "mlir/Dialect/ControlFlow/IR/ControlFlowOps.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "llvm/ADT/SmallVector.h"

mlir::LogicalResult MLIRGen::emitBlock(const CompoundStmt &block,
                                       mlir::func::FuncOp function) {
  for (const Node *item : block.items) {
    auto *insertionBlock = builder.getInsertionBlock();
    if (!insertionBlock->empty() &&
        insertionBlock->back().hasTrait<mlir::OpTrait::IsTerminator>())
      break;
    if (auto *var = dynamic_cast<const VarDecl *>(item)) {
      if (var->isGlobal || var->type->isArray() ||
          var->type->quals.isVolatile || var->sc == StorageClass::STATIC ||
          var->sc == StorageClass::EXTERN) {
        diag.error(var->loc, "MLIR generation currently supports only "
                             "automatic scalar local storage");
        return mlir::failure();
      }
      mlir::Type type = lowerType(*var->type, var->loc);
      if (!type)
        return mlir::failure();
      auto address =
          builder
              .create<mlir::memref::AllocaOp>(builder.getUnknownLoc(),
                                              mlir::MemRefType::get({}, type))
              .getResult();
      storage[var] = address;
      if (var->init) {
        mlir::Value value = emitExpr(*var->init);
        if (!value)
          return mlir::failure();
        if (value.getType() != type) {
          diag.error(var->loc,
                     "MLIR initializer type does not match the local variable");
          return mlir::failure();
        }
        builder.create<mlir::memref::StoreOp>(builder.getUnknownLoc(), value,
                                              address, mlir::ValueRange{});
      }
    } else if (dynamic_cast<const TypedefDecl *>(item)) {
      continue;
    } else if (auto *nested = dynamic_cast<const CompoundStmt *>(item)) {
      if (mlir::failed(emitBlock(*nested, function)))
        return mlir::failure();
    } else if (auto *stmt = dynamic_cast<const ExprStmt *>(item)) {
      if (!stmt->expr)
        continue;
      if (auto *call = dynamic_cast<const CallExpr *>(stmt->expr)) {
        if (mlir::failed(emitCall(*call)))
          return mlir::failure();
      } else if (!emitExpr(*stmt->expr)) {
        return mlir::failure();
      }
    } else if (auto *stmt = dynamic_cast<const IfStmt *>(item)) {
      if (mlir::failed(emitIf(*stmt, function)))
        return mlir::failure();
    } else if (auto *stmt = dynamic_cast<const WhileStmt *>(item)) {
      if (mlir::failed(emitWhile(*stmt, function)))
        return mlir::failure();
    } else if (auto *stmt = dynamic_cast<const BreakStmt *>(item)) {
      if (loopTargets.empty()) {
        diag.error(stmt->loc, "MLIR break outside loop");
        return mlir::failure();
      }

      builder.create<mlir::cf::BranchOp>(builder.getUnknownLoc(),
                                         loopTargets.back().breakTarget);

    } else if (auto *stmt = dynamic_cast<const ContinueStmt *>(item)) {
      if (loopTargets.empty()) {
        diag.error(stmt->loc, "MLIR continue outside loop");
        return mlir::failure();
      }

      builder.create<mlir::cf::BranchOp>(builder.getUnknownLoc(),
                                         loopTargets.back().continueTarget);
    } else if (auto *ret = dynamic_cast<const ReturnStmt *>(item)) {
      llvm::SmallVector<mlir::Value> operands;
      if (ret->value) {
        mlir::Value value = emitExpr(*ret->value);
        if (!value)
          return mlir::failure();
        operands.push_back(value);
      }
      if (operands.size() != function.getNumResults() ||
          (!operands.empty() &&
           operands.front().getType() != function.getResultTypes().front())) {
        diag.error(ret->loc,
                   "MLIR return types do not match the function signature");
        return mlir::failure();
      }
      builder.create<mlir::func::ReturnOp>(builder.getUnknownLoc(), operands);
    } else {
      diag.error(item->loc,
                 "MLIR generation does not support this block item yet");
      return mlir::failure();
    }
  }
  return mlir::success();
}
