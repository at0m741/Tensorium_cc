#include "cc1/CodeGen/MLIRGen.hpp"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlowOps.h"

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
      mlir::arith::ConstantIntOp::create(builder, loc, 0, type.getWidth());
  return mlir::arith::CmpIOp::create(builder, loc,
                                     mlir::arith::CmpIPredicate::ne, value,
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

  mlir::cf::CondBranchOp::create(builder, loc, condition, thenBlock,
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
    mlir::cf::BranchOp::create(builder, loc, mergeBlock);

  bool elseContinues = true;
  if (elseBlock) {
    builder.setInsertionPointToStart(elseBlock);

    if (mlir::failed(emitBranch(stmt.els)))
      return mlir::failure();

    elseContinues = !terminated();
    if (elseContinues)
      mlir::cf::BranchOp::create(builder, loc, mergeBlock);
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
  mlir::cf::BranchOp::create(builder, loc, conditionBlock);

  builder.setInsertionPointToStart(conditionBlock);
  mlir::Value condition = emitCondition(*stmt.cond);
  if (!condition)
    return mlir::failure();

  mlir::cf::CondBranchOp::create(builder, loc, condition, bodyBlock, exitBlock);

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
    mlir::cf::BranchOp::create(builder, loc, conditionBlock);
  }

  builder.setInsertionPointToStart(exitBlock);
  return mlir::success();
}
