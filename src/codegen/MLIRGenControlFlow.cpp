#include "MLIRGen.hpp"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlowOps.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"

void MLIRGen::materializeParameters() {
  auto loc = builder.getUnknownLoc();

  for (const auto &binding : values) {
    if (storage.find(binding.first) != storage.end())
      continue;

    auto type = mlir::MemRefType::get({}, binding.second.getType());
    auto slot = builder.create<mlir::memref::AllocaOp>(loc, type).getResult();

    builder.create<mlir::memref::StoreOp>(loc, binding.second, slot,
                                          mlir::ValueRange{});

    storage[binding.first] = slot;
  }
}

mlir::Value MLIRGen::emitCondition(const Expr &expr) {
  mlir::Value value = emitExpr(expr);
  if (!value)
    return {};

  auto type = mlir::dyn_cast<mlir::IntegerType>(value.getType());
  if (!type) {
    diag.error(expr.loc, "MLIR conditions currently require an integer value");
    return {};
  }

  if (type.getWidth() == 1)
    return value;

  auto loc = builder.getUnknownLoc();

  auto zero =
      builder.create<mlir::arith::ConstantIntOp>(loc, 0, type.getWidth());
  return builder
      .create<mlir::arith::CmpIOp>(loc, mlir::arith::CmpIPredicate::ne, value,
                                   zero.getResult())
      .getResult();
}

mlir::LogicalResult MLIRGen::emitIf(const IfStmt &stmt,
                                    mlir::func::FuncOp function) {
  auto loc = builder.getUnknownLoc();

  materializeParameters();

  mlir::Value condition = emitCondition(*stmt.cond);
  if (!condition)
    return mlir::failure();

  auto parentPoint = builder.saveInsertionPoint();

  auto *thenBlock = builder.createBlock(&function.getBody());
  auto *elseBlock =
      stmt.els ? builder.createBlock(&function.getBody()) : nullptr;
  auto *mergeBlock = builder.createBlock(&function.getBody());

  builder.restoreInsertionPoint(parentPoint);

  builder.create<mlir::cf::CondBranchOp>(loc, condition, thenBlock,
                                         elseBlock ? elseBlock : mergeBlock);

  auto emitBranch = [&](Stmt *branch) -> mlir::LogicalResult {
    CompoundStmt wrapper;
    wrapper.items.push_back(branch);
    return emitBlock(wrapper, function);
  };

  auto terminated = [&]() {
    auto *block = builder.getInsertionBlock();
    return !block->empty() &&
           block->back().hasTrait<mlir::OpTrait::IsTerminator>();
  };

  builder.setInsertionPointToStart(thenBlock);
  if (mlir::failed(emitBranch(stmt.then)))
    return mlir::failure();

  bool thenContinues = !terminated();
  if (thenContinues)
    builder.create<mlir::cf::BranchOp>(loc, mergeBlock);

  bool elseContinues = true;
  if (elseBlock) {
    builder.setInsertionPointToStart(elseBlock);

    if (mlir::failed(emitBranch(stmt.els)))
      return mlir::failure();

    elseContinues = !terminated();
    if (elseContinues)
      builder.create<mlir::cf::BranchOp>(loc, mergeBlock);
  }

  if (!thenContinues && !elseContinues) {
    mergeBlock->erase();
    return mlir::success();
  }

  builder.setInsertionPointToStart(mergeBlock);
  return mlir::success();
}

mlir::LogicalResult MLIRGen::emitWhile(const WhileStmt &stmt,
                                       mlir::func::FuncOp function) {
  auto loc = builder.getUnknownLoc();

  materializeParameters();

  auto parentPoint = builder.saveInsertionPoint();

  auto *conditionBlock = builder.createBlock(&function.getBody());
  auto *bodyBlock = builder.createBlock(&function.getBody());
  auto *exitBlock = builder.createBlock(&function.getBody());

  builder.restoreInsertionPoint(parentPoint);
  builder.create<mlir::cf::BranchOp>(loc, conditionBlock);

  builder.setInsertionPointToStart(conditionBlock);
  mlir::Value condition = emitCondition(*stmt.cond);
  if (!condition)
    return mlir::failure();

  builder.create<mlir::cf::CondBranchOp>(loc, condition, bodyBlock, exitBlock);

  builder.setInsertionPointToStart(bodyBlock);
  loopTargets.push_back({exitBlock, conditionBlock});
  CompoundStmt wrapper;
  wrapper.items.push_back(stmt.body);
  mlir::LogicalResult result = emitBlock(wrapper, function);

  loopTargets.pop_back();

  if (mlir::failed(result))
    return mlir::failure();
  auto *currentBlock = builder.getInsertionBlock();
  if (currentBlock->empty() ||
      !currentBlock->back().hasTrait<mlir::OpTrait::IsTerminator>()) {
    builder.create<mlir::cf::BranchOp>(loc, conditionBlock);
  }

  builder.setInsertionPointToStart(exitBlock);
  return mlir::success();
}
