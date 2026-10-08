#include "MLIRGen.hpp"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlowOps.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/IR/Verifier.h"
#include "parser/AST.hpp"
#include "llvm/ADT/DenseMap.h"
#include <llvm/ADT/SmallVector.h>
#include <mlir/IR/Builders.h>
#include <mlir/IR/ValueRange.h>
#include <mlir/Support/LLVM.h>

MLIRGen::MLIRGen(mlir::MLIRContext &context, DiagnosticEngine &diagnostics,
                 const TargetInfo &targetInfo)
    : builder(&context), diag(diagnostics), target(targetInfo) {
  context.loadDialect<mlir::arith::ArithDialect, mlir::func::FuncDialect,
                      mlir::memref::MemRefDialect,
                      mlir::cf::ControlFlowDialect>();
}

mlir::OwningOpRef<mlir::ModuleOp>
MLIRGen::generate(const TranslationUnit &unit) {
  mlir::OwningOpRef<mlir::ModuleOp> module(
      mlir::ModuleOp::create(builder.getUnknownLoc()));

  functions.clear();
  values.clear();
  storage.clear();
  llvm::DenseMap<const Decl *, const FuncDecl *> selected;
  llvm::SmallVector<const Decl *> order;
  for (const Decl *decl : unit.decls) {
    if (dynamic_cast<const TypedefDecl *>(decl))
      continue;
    auto *function = dynamic_cast<const FuncDecl *>(decl);
    if (!function) {
      diag.error(decl->loc, "MLIR generation is not implemented "
                            "for this declaration yet");
      return {};
    }
    const Decl *key =
        function->canonicalDecl ? function->canonicalDecl : function;
    auto found = selected.find(key);
    if (found == selected.end()) {
      order.push_back(key);
      selected[key] = function;
    } else if (function->body ||
               (!found->second->body && function->type->hasPrototype)) {
      selected[key] = function;
    }
  }
  // Declare symbols first, so prototypes, recursion and later definitions share
  // the canonical declaration established by Sema.
  for (const Decl *key : order) {
    builder.setInsertionPointToEnd(module->getBody());
    if (mlir::failed(declareFunction(*selected[key])))
      return {};
  }
  for (const Decl *key : order) {
    const FuncDecl &function = *selected[key];
    if (function.body && mlir::failed(emitFunction(function)))
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
    if (literal->type && literal->type->isIntegral()) {
      mlir::Type type = lowerType(*literal->type, literal->loc);
      if (!type)
        return {};

      auto intType = mlir::dyn_cast<mlir::IntegerType>(type);
      if (!intType) {
        diag.error(literal->loc,
                   "integer literal lowered to a non-integer MLIR type");
        return {};
      }

      return builder
          .create<mlir::arith::ConstantIntOp>(
              builder.getUnknownLoc(), static_cast<int64_t>(literal->val),
              intType.getWidth())
          .getResult();
    }
  }

  if (auto *ref = dynamic_cast<const IdentExpr *>(&expr)) {
    auto slot = storage.find(ref->decl);
    if (slot != storage.end())
      return builder
          .create<mlir::memref::LoadOp>(builder.getUnknownLoc(), slot->second,
                                        mlir::ValueRange{})
          .getResult();
    auto it = values.find(ref->decl);
    if (it == values.end()) {
      diag.error(ref->loc, "unresolved declaration in MLIR generation");
      return {};
    }
    return it->second;
  }

  if (auto *cast = dynamic_cast<const ImplicitCastExpr *>(&expr)) {
    if (!cast->operand || !cast->operand->type || !cast->type) {
      diag.error(cast->loc, "untyped implicit conversion in MLIR generation");
      return {};
    }
    if (cast->kind == ImplicitCastKind::LValueToRValue)
      return emitExpr(*cast->operand);

    if (cast->kind == ImplicitCastKind::Arithmetic &&
        cast->operand->type->isIntegral() && cast->type->isIntegral()) {
      mlir::Value value = emitExpr(*cast->operand);
      if (!value)
        return {};
      auto from = mlir::dyn_cast<mlir::IntegerType>(value.getType());
      mlir::Type destination = lowerType(*cast->type, cast->loc);
      if (!destination)
        return {};
      auto to = mlir::dyn_cast<mlir::IntegerType>(destination);
      if (!from || !to) {
        diag.error(cast->loc, "integer conversion requires integer MLIR types");
        return {};
      }
      auto loc = builder.getUnknownLoc();
      if (cast->type->kind == Type::BOOL) {
        auto zero = builder.create<mlir::arith::ConstantIntOp>(loc, 0, from);
        return builder
            .create<mlir::arith::CmpIOp>(loc, mlir::arith::CmpIPredicate::ne,
                                         value, zero.getResult())
            .getResult();
      }
      if (from == to)
        return value;
      if (from.getWidth() > to.getWidth())
        return builder.create<mlir::arith::TruncIOp>(loc, to, value)
            .getResult();
      if (cast->operand->type->isSigned())
        return builder.create<mlir::arith::ExtSIOp>(loc, to, value).getResult();
      return builder.create<mlir::arith::ExtUIOp>(loc, to, value).getResult();
    }
    diag.error(cast->loc,
               "MLIR generation does not support this implicit conversion yet");
    return {};
  }

  if (auto *call = dynamic_cast<const CallExpr *>(&expr)) {
    mlir::Value result;
    if (mlir::failed(emitCall(*call, &result)))
      return {};
    return result;
  }

  if (auto *binary = dynamic_cast<const BinaryExpr *>(&expr)) {
    if (binary->op == BinaryOp::ASSIGN) {
      auto *ref = dynamic_cast<const IdentExpr *>(binary->lhs);
      if (!ref || !ref->decl) {
        diag.error(binary->loc, "MLIR assignment requires a scalar variable");
        return {};
      }
      auto slot = storage.find(ref->decl);
      if (slot == storage.end()) {
        auto value = values.find(ref->decl);
        if (value == values.end()) {
          diag.error(ref->loc,
                     "unresolved assignment target in MLIR generation");
          return {};
        }
        auto type = mlir::MemRefType::get({}, value->second.getType());
        auto address =
            builder
                .create<mlir::memref::AllocaOp>(builder.getUnknownLoc(), type)
                .getResult();
        builder.create<mlir::memref::StoreOp>(builder.getUnknownLoc(),
                                              value->second, address,
                                              mlir::ValueRange{});
        storage[ref->decl] = address;
      }
      mlir::Value value = emitExpr(*binary->rhs);
      if (!value)
        return {};
      mlir::Value address = storage.lookup(ref->decl);
      auto type = mlir::cast<mlir::MemRefType>(address.getType());
      if (value.getType() != type.getElementType()) {
        diag.error(binary->loc, "MLIR assignment types do not match the AST");
        return {};
      }
      builder.create<mlir::memref::StoreOp>(builder.getUnknownLoc(), value,
                                            address, mlir::ValueRange{});
      return value;
    }
    if (binary->op != BinaryOp::ADD && binary->op != BinaryOp::SUB &&
        binary->op != BinaryOp::MUL) {
      diag.error(binary->loc,
                 "MLIR generation does not support this binary operator yet");
      return {};
    }
    if (!binary->type || !binary->type->isIntegral() || !binary->lhs ||
        !binary->rhs) {
      diag.error(binary->loc,
                 "MLIR binary arithmetic currently requires integer operands");
      return {};
    }
    mlir::Value lhs = emitExpr(*binary->lhs);
    if (!lhs)
      return {};
    mlir::Value rhs = emitExpr(*binary->rhs);
    if (!rhs)
      return {};
    mlir::Type resultType = lowerType(*binary->type, binary->loc);
    if (!resultType || !mlir::isa<mlir::IntegerType>(lhs.getType()) ||
        lhs.getType() != rhs.getType() || lhs.getType() != resultType) {
      diag.error(binary->loc, "MLIR binary operand types do not match the AST");
      return {};
    }
    auto loc = builder.getUnknownLoc();
    switch (binary->op) {
    case BinaryOp::ADD:
      return builder.create<mlir::arith::AddIOp>(loc, lhs, rhs).getResult();
    case BinaryOp::SUB:
      return builder.create<mlir::arith::SubIOp>(loc, lhs, rhs).getResult();
    case BinaryOp::MUL:
      return builder.create<mlir::arith::MulIOp>(loc, lhs, rhs).getResult();
    default:
      break;
    }
  }

  diag.error(expr.loc, "MLIR generation does not support this expression yet");
  return {};
}

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
  op.setPrivate(); // External declarations must have private MLIR visibility.
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

mlir::LogicalResult MLIRGen::emitIf(const IfStmt &stmt,
                                    mlir::func::FuncOp function) {
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
