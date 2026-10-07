#pragma once

#include "cc1/Diagnostic.hpp"
#include "parser/AST.hpp"

#include "cc1/TargetInfo.hpp"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/IR/OwningOpRef.h"
#include "parser/Type.hpp"

#include "llvm/ADT/DenseMap.h"
#include <mlir/IR/Value.h>

class MLIRGen {
public:
  MLIRGen(mlir::MLIRContext &context, DiagnosticEngine &diag,
          const TargetInfo &target);

  mlir::OwningOpRef<mlir::ModuleOp> generate(const TranslationUnit &unit);

private:
  mlir::OpBuilder builder;
  DiagnosticEngine &diag;
  const TargetInfo &target;

  llvm::DenseMap<const Decl *, mlir::Value> values;
  llvm::DenseMap<const Decl *, mlir::Value> storage;
  llvm::DenseMap<const Decl *, mlir::func::FuncOp> functions;
  mlir::Type lowerType(const ::Type &type, SourceLoc loc);
  mlir::LogicalResult declareFunction(const FuncDecl &function);
  mlir::LogicalResult emitFunction(const FuncDecl &function);
  mlir::LogicalResult emitBlock(const CompoundStmt &block,
                                mlir::func::FuncOp function);
  mlir::LogicalResult emitCall(const CallExpr &call,
                               mlir::Value *result = nullptr);
  mlir::Value emitExpr(const Expr &expr);
};
