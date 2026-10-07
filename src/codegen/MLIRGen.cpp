#include "MLIRGen.hpp"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/SCF/IR/SCFDialect.h"
#include "mlir/IR/Verifier.h"

MLIRGen::MLIRGen(mlir::MLIRContext &context, DiagnosticEngine &diagnostics,
                 const TargetInfo &targetInfo)
    : builder(&context), diag(diagnostics), target(targetInfo) {
  context.loadDialect<mlir::arith::ArithDialect, mlir::func::FuncDialect,
                      mlir::scf::SCFDialect>();
}

mlir::OwningOpRef<mlir::ModuleOp>
MLIRGen::generate(const TranslationUnit &unit) {
  mlir::OwningOpRef<mlir::ModuleOp> module(
      mlir::ModuleOp::create(builder.getUnknownLoc()));

  builder.setInsertionPointToEnd(module->getBody());

  for (const Decl *decl : unit.decls) {
    auto *function = dynamic_cast<const FuncDecl *>(decl);
    if (!function) {
      diag.error(decl->loc, "MLIR generation is not implemented "
                            "for this declaration yet");
      return {};
    }

    builder.setInsertionPointToEnd(module->getBody());

    if (mlir::failed(emitFunction(*function)))
      return {};
  }

  if (mlir::failed(mlir::verify(module.get()))) {
    diag.error(unit.loc, "generated MLIR module is invalid");
    return {};
  }

  return module;
}

mlir::Type MLIRGen::lowerType(const ::Type &type, SourceLoc loc) {
  switch (type.kind) {
  case Type::VOID:
    return builder.getNoneType();
  case Type::BOOL:
    return builder.getI1Type();
  case Type::CHAR:
  case Type::SCHAR:
  case Type::UCHAR:
    return builder.getIntegerType(8);

  case Type::SHORT:
  case Type::USHORT:
    return builder.getIntegerType(target.sizeofShort * 8);

  case Type::INT:
  case Type::UINT:
    return builder.getIntegerType(target.sizeofInt * 8);

  case Type::LONG:
  case Type::ULONG:
    return builder.getIntegerType(target.sizeofLong * 8);

  case Type::LONGLONG:
  case Type::ULONGLONG:
    return builder.getIntegerType(target.sizeofLonglong * 8);

  case Type::FLOAT:
    return builder.getF32Type();

  case Type::DOUBLE:
    return builder.getF64Type();

  default:
    diag.error(loc, "MLIR generation does not support type '" + type.str() +
                        "' yet");
    return {};
  }
}

mlir::Value MLIRGen::emitExpr(const Expr &expr) {
  if (auto *literal = dynamic_cast<const IntLitExpr *>(&expr)) {
    if (literal->type && literal->type->kind == Type::INT) {
      mlir::Type type = lowerType(*literal->type, literal->loc);
      if (!type)
        return {};

      auto intType = mlir::dyn_cast<mlir::IntegerType>(type);
      if (!intType) {
        diag.error(literal->loc,
                   "integer literal lowered to a non-integer MLIR type");
        return {};
      }

      return mlir::arith::ConstantIntOp::create(
                 builder, builder.getUnknownLoc(),
                 static_cast<int64_t>(literal->val), intType.getWidth())
          .getResult();
    }
  }

  diag.error(expr.loc, "MLIR generation does not support this expression yet");
  return {};
}

mlir::LogicalResult MLIRGen::emitFunction(const FuncDecl &function) {
  if (!function.body || !function.type->params.empty() ||
      function.type->variadic || function.type->retType->kind != Type::INT ||
      function.body->items.size() != 1) {
    diag.error(function.loc, "MLIR generation is not implemented "
                             "for this declaration yet");
    return mlir::failure();
  }

  auto *returnStmt =
      dynamic_cast<const ReturnStmt *>(function.body->items.front());

  if (!returnStmt || !returnStmt->value) {
    diag.error(function.loc,
               "MLIR generation requires a single return with a value");
    return mlir::failure();
  }

  mlir::Type resultType = lowerType(*function.type->retType, function.loc);
  if (!resultType)
    return mlir::failure();

  mlir::OpBuilder::InsertionGuard guard(builder);

  auto location = builder.getUnknownLoc();
  auto signature = builder.getFunctionType({}, {resultType});

  auto mlirFunction =
      mlir::func::FuncOp::create(builder, location, function.name, signature);

  if (function.sc == StorageClass::STATIC)
    mlirFunction.setPrivate();

  mlir::Block *entry = mlirFunction.addEntryBlock();
  builder.setInsertionPointToStart(entry);

  mlir::Value value = emitExpr(*returnStmt->value);
  if (!value) {
    mlirFunction.erase();
    return mlir::failure();
  }

  if (value.getType() != resultType) {
    diag.error(returnStmt->loc,
               "MLIR return conversion is not implemented yet");
    mlirFunction.erase();
    return mlir::failure();
  }

  mlir::func::ReturnOp::create(builder, location, mlir::ValueRange{value});

  return mlir::success();
}
