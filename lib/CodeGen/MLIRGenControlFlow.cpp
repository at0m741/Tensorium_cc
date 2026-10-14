#include "cc1/CodeGen/MLIRGen.hpp"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlowOps.h"
#include <mlir/Support/LLVM.h>

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

#if LLVM_VERSION_MAJOR >= 22
  auto zero =
      mlir::arith::ConstantIntOp::create(builder, loc, 0, type.getWidth());
#else
  auto zero =
      builder.create<mlir::arith::ConstantIntOp>(loc, 0, type.getWidth());
#endif
#if LLVM_VERSION_MAJOR >= 22
  return mlir::arith::CmpIOp::create(builder, loc,
                                     mlir::arith::CmpIPredicate::ne, value,
                                     zero.getResult())
      .getResult();
#else
  return builder
      .create<mlir::arith::CmpIOp>(loc, mlir::arith::CmpIPredicate::ne, value,
                                   zero.getResult())
      .getResult();
#endif
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

#if LLVM_VERSION_MAJOR >= 22
  mlir::cf::CondBranchOp::create(builder, loc, condition, thenBlock,
                                 elseBlock ? elseBlock : mergeBlock);
#else
  builder.create<mlir::cf::CondBranchOp>(loc, condition, thenBlock,
                                         elseBlock ? elseBlock : mergeBlock);
#endif

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
#if LLVM_VERSION_MAJOR >= 22
    mlir::cf::BranchOp::create(builder, loc, mergeBlock);
#else
    builder.create<mlir::cf::BranchOp>(loc, mergeBlock);
#endif

  bool elseContinues = true;
  if (elseBlock) {
    builder.setInsertionPointToStart(elseBlock);

    if (mlir::failed(emitBranch(stmt.els)))
      return mlir::failure();

    elseContinues = !terminated();
    if (elseContinues)
#if LLVM_VERSION_MAJOR >= 22
      mlir::cf::BranchOp::create(builder, loc, mergeBlock);
#else
      builder.create<mlir::cf::BranchOp>(loc, mergeBlock);
#endif
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
#if LLVM_VERSION_MAJOR >= 22
  mlir::cf::BranchOp::create(builder, loc, conditionBlock);
#else
  builder.create<mlir::cf::BranchOp>(loc, conditionBlock);
#endif

  builder.setInsertionPointToStart(conditionBlock);
  mlir::Value condition = emitCondition(*stmt.cond);
  if (!condition)
    return mlir::failure();

#if LLVM_VERSION_MAJOR >= 22
  mlir::cf::CondBranchOp::create(builder, loc, condition, bodyBlock, exitBlock);
#else
  builder.create<mlir::cf::CondBranchOp>(loc, condition, bodyBlock, exitBlock);
#endif

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
#if LLVM_VERSION_MAJOR >= 22
    mlir::cf::BranchOp::create(builder, loc, conditionBlock);
#else
    builder.create<mlir::cf::BranchOp>(loc, conditionBlock);
#endif
  }

  builder.setInsertionPointToStart(exitBlock);
  return mlir::success();
}

mlir::LogicalResult MLIRGen::emitFor(const ForStmt &stmt,
                                     mlir::func::FuncOp function) {
  auto loc = builder.getUnknownLoc();

  materializeParameters();

  if (stmt.init) {
    CompoundStmt initializer;
    initializer.items.push_back(stmt.init);
    if (mlir::failed(emitBlock(initializer, function)))
      return mlir::failure();
  }
  auto parentPoint = builder.saveInsertionPoint();

  auto *conditionBlock = builder.createBlock(&function.getBody());
  auto *bodyBlock = builder.createBlock(&function.getBody());
  auto *incrementBlock = builder.createBlock(&function.getBody());
  auto *exitBlock = builder.createBlock(&function.getBody());

  auto branchTo = [&](mlir::Block *destination) {
#if LLVM_VERSION_MAJOR >= 22
    mlir::cf::BranchOp::create(builder, loc, destination);
#else
    builder.create<mlir::cf::BranchOp>(loc, destination);
#endif
  };

  builder.restoreInsertionPoint(parentPoint);
  branchTo(conditionBlock);

  builder.setInsertionPointToStart(conditionBlock);
  if (stmt.cond) {
    mlir::Value condition = emitCondition(*stmt.cond);
    if (!condition)
      return mlir::failure();

#if LLVM_VERSION_MAJOR >= 22
    mlir::cf::CondBranchOp::create(builder, loc, condition, bodyBlock,
                                   exitBlock);
#else
    builder.create<mlir::cf::CondBranchOp>(loc, condition, bodyBlock,
                                           exitBlock);
#endif
  } else {
    branchTo(bodyBlock);
  }

  builder.setInsertionPointToStart(bodyBlock);
  loopTargets.push_back({exitBlock, incrementBlock});

  CompoundStmt body;
  body.items.push_back(stmt.body);
  mlir::LogicalResult result = emitBlock(body, function);

  loopTargets.pop_back();
  if (mlir::failed(result))
    return mlir::failure();

  auto *currentBlock = builder.getInsertionBlock();
  if (currentBlock->empty() ||
      !currentBlock->back().hasTrait<mlir::OpTrait::IsTerminator>())
    branchTo(incrementBlock);

  builder.setInsertionPointToStart(incrementBlock);

  if (stmt.incr) {
    ExprStmt increment;
    increment.expr = stmt.incr;
    increment.loc = stmt.incr->loc;

    CompoundStmt incrementBody;
    incrementBody.items.push_back(&increment);
    if (mlir::failed(emitBlock(incrementBody, function)))
      return mlir::failure();
  }

  branchTo(conditionBlock);

  builder.setInsertionPointToStart(exitBlock);
  return mlir::success();
}
