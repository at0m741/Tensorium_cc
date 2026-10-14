#pragma once

#include "cc1/AST/AST.hpp"
#include "cc1/AST/Type.hpp"
#include "cc1/Basic/Diagnostic.hpp"
#include "cc1/Basic/TargetInfo.hpp"
#include "cc1/CodeGen/Address.hpp"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/IR/OwningOpRef.h"

#include "llvm/ADT/DenseMap.h"
#include "llvm/Config/llvm-config.h"
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
  llvm::DenseMap<const Decl *, Address> storage;
  llvm::DenseMap<const Decl *, mlir::func::FuncOp> functions;

  Address createScalarStorage(mlir::Type type, const Type &objectType);
  Address emitLValue(const Expr &expr);
  mlir::Value emitLoad(Address address);
  void emitStore(mlir::Value value, Address address);
  void materializeParameters();

  mlir::Type lowerType(const ::Type &type, SourceLoc loc);
  mlir::Value emitExpr(const Expr &expr);
  mlir::Value emitBinary(const BinaryExpr &binary);

  // Function declarations, definitions and calls.
  mlir::LogicalResult declareFunction(const FuncDecl &function);
  mlir::LogicalResult emitFunction(const FuncDecl &function);
  mlir::LogicalResult emitCall(const CallExpr &call,
                               mlir::Value *result = nullptr);

  // Statements and control flow.
  mlir::LogicalResult emitBlock(const CompoundStmt &block,
                                mlir::func::FuncOp function);
  mlir::LogicalResult emitIf(const IfStmt &stmt, mlir::func::FuncOp function);
  mlir::Value emitCondition(const Expr &expr);
  mlir::LogicalResult emitWhile(const WhileStmt &stmt,
                                mlir::func::FuncOp function);
  mlir::LogicalResult emitFor(const ForStmt &stmt, mlir::func::FuncOp function);
};
