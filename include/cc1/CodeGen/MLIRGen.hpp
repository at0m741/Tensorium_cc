#pragma once

#include "cc1/AST/AST.hpp"
#include "cc1/Basic/Diagnostic.hpp"

#include "cc1/AST/Type.hpp"
#include "cc1/Basic/TargetInfo.hpp"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/IR/OwningOpRef.h"

#include "llvm/ADT/DenseMap.h"
#include <mlir/IR/Value.h>
#include <vector>

class MLIRGen {
public:
  MLIRGen(mlir::MLIRContext &context, DiagnosticEngine &diag,
          const TargetInfo &target);

  mlir::OwningOpRef<mlir::ModuleOp> generate(const TranslationUnit &unit);

private:
  mlir::OpBuilder builder;
  DiagnosticEngine &diag;
  const TargetInfo &target;

  struct LoopTargets {
    mlir::Block *breakTarget;
    mlir::Block *continueTarget;
  };

  std::vector<LoopTargets> loopTargets;

  llvm::DenseMap<const Decl *, mlir::Value> values;
  llvm::DenseMap<const Decl *, mlir::Value> storage;
  llvm::DenseMap<const Decl *, mlir::func::FuncOp> functions;

  mlir::Value createScalarStorage(mlir::Type type);
  mlir::Value emitLValue(const Expr &expr);
  mlir::Value emitLoad(mlir::Value address);
  void emitStore(mlir::Value value, mlir::Value address);
  void materializeParameters();

  mlir::Type lowerType(const ::Type &type, SourceLoc loc);
  mlir::LogicalResult declareFunction(const FuncDecl &function);
  mlir::LogicalResult emitFunction(const FuncDecl &function);
  mlir::LogicalResult emitBlock(const CompoundStmt &block,
                                mlir::func::FuncOp function);
  mlir::LogicalResult emitCall(const CallExpr &call,
                               mlir::Value *result = nullptr);
  mlir::Value emitExpr(const Expr &expr);
  mlir::LogicalResult emitIf(const IfStmt &stmt, mlir::func::FuncOp function);
  mlir::Value emitCondition(const Expr &expr);
  mlir::LogicalResult emitWhile(const WhileStmt &stmt,
                                mlir::func::FuncOp function);
};
