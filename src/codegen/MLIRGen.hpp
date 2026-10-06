#pragma once

#include "cc1/Diagnostic.hpp"
#include "parser/AST.hpp"

#include "cc1/TargetInfo.hpp"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/IR/OwningOpRef.h"
#include "parser/Type.hpp"

class MLIRGen {
public:
  MLIRGen(mlir::MLIRContext &context, DiagnosticEngine &diag,
          const TargetInfo &target);

  mlir::OwningOpRef<mlir::ModuleOp> generate(const TranslationUnit &unit);

private:
  mlir::OpBuilder builder;
  DiagnosticEngine &diag;
  const TargetInfo &target;

  mlir::Type lowerType(const ::Type &type, SourceLoc loc);
  mlir::LogicalResult emitFunction(const FuncDecl &function);
  mlir::Value emitExpr(const Expr &expr);
};
