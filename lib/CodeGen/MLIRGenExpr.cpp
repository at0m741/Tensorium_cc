#include "cc1/AST/AST.hpp"
#include "cc1/CodeGen/MLIRGen.hpp"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "tensorium/Dialect/Tensorium/IR/TensoriumOps.h"
#include "tensorium/Dialect/Tensorium/IR/TensoriumTypes.h"
#include "tensorium/Support/Builders.h"
#include "llvm/ADT/SmallVector.h"
#include <limits>
#include <optional>

namespace {
std::optional<uint64_t> objectSize(const Type &type, const TargetInfo &target) {
  switch (type.kind) {
  case Type::BOOL:
  case Type::CHAR:
  case Type::SCHAR:
  case Type::UCHAR:
    return 1;
  case Type::SHORT:
  case Type::USHORT:
    return target.sizeofShort;
  case Type::INT:
  case Type::UINT:
    return target.sizeofInt;
  case Type::LONG:
  case Type::ULONG:
    return target.sizeofLong;
  case Type::LONGLONG:
  case Type::ULONGLONG:
    return target.sizeofLonglong;
  case Type::FLOAT:
    return target.sizeofFloat;
  case Type::DOUBLE:
    return target.sizeofDouble;
  case Type::POINTER:
    return target.sizeofPointer;
  case Type::ARRAY: {
    if (!type.elemType || type.arraySize <= 0)
      return std::nullopt;
    auto element = objectSize(*type.elemType, target);
    if (!element ||
        *element > std::numeric_limits<uint64_t>::max() / type.arraySize)
      return std::nullopt;
    return *element * type.arraySize;
  }
  default:
    return std::nullopt;
  }
}
} // namespace

mlir::Value MLIRGen::emitExpr(const Expr &expr) {
  using tensorium::createOp;
  if (expr.type && expr.type->isArray() && expr.isLval) {
    Address address = emitLValue(expr);
    return address ? address.base : mlir::Value{};
  }
  if (auto *size = dynamic_cast<const SizeofExpr *>(&expr)) {
    const Type *object = size->ofType ? size->operandType : size->expr->type;
    auto type = lowerType(*expr.type, expr.loc);
    auto bytes = objectSize(*object, target);
    if (!type || !bytes ||
        (target.sizeofPointer < 8 &&
         *bytes >= (uint64_t{1} << (target.sizeofPointer * 8)))) {
      diag.error(expr.loc,
                 "MLIR sizeof requires a supported complete object type");
      return {};
    }
    return createOp<mlir::arith::ConstantOp>(
               builder, builder.getUnknownLoc(),
               builder.getIntegerAttr(type, *bytes))
        .getResult();
  }
  if (dynamic_cast<const IndexExpr *>(&expr)) {
    Address address = emitLValue(expr);
    return address ? emitLoad(address) : mlir::Value{};
  }
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

#if LLVM_VERSION_MAJOR >= 22
      return mlir::arith::ConstantIntOp::create(
                 builder, builder.getUnknownLoc(),
                 static_cast<int64_t>(literal->val), intType.getWidth())
          .getResult();
#else
      return builder
          .create<mlir::arith::ConstantIntOp>(
              builder.getUnknownLoc(), static_cast<int64_t>(literal->val),
              intType.getWidth())
          .getResult();
#endif
    }
  }

  if (auto *ref = dynamic_cast<const IdentExpr *>(&expr)) {
    const Decl *key = ref->decl && ref->decl->canonicalDecl
                          ? ref->decl->canonicalDecl
                          : ref->decl;
    if (globals.count(key)) {
      auto address = emitLValue(expr);
      return address ? emitLoad(address) : mlir::Value{};
    }
    auto slot = storage.find(ref->decl);
    if (slot != storage.end())
      return emitLoad(slot->second);
    auto it = values.find(ref->decl);
    if (it == values.end()) {
      diag.error(ref->loc, "unresolved declaration in MLIR generation");
      return {};
    }
    return it->second;
  }

  if (auto *unary = dynamic_cast<const UnaryExpr *>(&expr)) {
    if (unary->op == UnaryOp::PRE_INC || unary->op == UnaryOp::PRE_DEC ||
        unary->op == UnaryOp::POST_INC || unary->op == UnaryOp::POST_DEC)
      return emitUpdate(*unary);
    if (unary->op == UnaryOp::NOT) {
      auto condition = emitCondition(*unary->operand);
      if (!condition)
        return {};
      auto loc = builder.getUnknownLoc();
      auto zero = createOp<mlir::arith::ConstantOp>(
          builder, loc, builder.getIntegerAttr(builder.getI1Type(), 0));
      auto inverted = createOp<mlir::arith::CmpIOp>(
          builder, loc, mlir::arith::CmpIPredicate::eq, condition,
          zero.getResult());
      return createOp<mlir::arith::ExtUIOp>(builder, loc,
                                            lowerType(*expr.type, expr.loc),
                                            inverted.getResult())
          .getResult();
    }
    if ((unary->op == UnaryOp::NEG || unary->op == UnaryOp::POS) &&
        expr.type->isIntegral()) {
      auto value = emitExpr(*unary->operand);
      if (!value || unary->op == UnaryOp::POS)
        return value;
      auto loc = builder.getUnknownLoc();
      auto zero = createOp<mlir::arith::ConstantOp>(
          builder, loc, builder.getIntegerAttr(value.getType(), 0));
      return createOp<mlir::arith::SubIOp>(builder, loc, zero.getResult(),
                                           value)
          .getResult();
    }
    if (unary->op == UnaryOp::ADDR) {
      Address address = emitLValue(*unary->operand);
      return address ? address.base : mlir::Value{};
    }

    if (unary->op == UnaryOp::DEREF) {
      Address address = emitLValue(expr);
      return address ? emitLoad(address) : mlir::Value{};
    }
  }

  if (auto *cast = dynamic_cast<const ImplicitCastExpr *>(&expr)) {
    if (!cast->operand || !cast->operand->type || !cast->type) {
      diag.error(cast->loc, "untyped implicit conversion in MLIR generation");
      return {};
    }
    if (cast->kind == ImplicitCastKind::LValueToRValue)
      return emitExpr(*cast->operand);

    if (cast->kind == ImplicitCastKind::ArrayToPointer) {
      Address array = emitLValue(*cast->operand);
      auto type = lowerType(*cast->type, cast->loc);
      if (!array || !type)
        return {};
      return createOp<tensorium::ArrayDecayOp>(builder, builder.getUnknownLoc(),
                                               type, array.base)
          .getResult();
    }
    if (cast->kind == ImplicitCastKind::NullToPointer) {
      auto type = lowerType(*cast->type, cast->loc);
      return type ? createOp<tensorium::NullOp>(builder,
                                                builder.getUnknownLoc(), type)
                        .getResult()
                  : mlir::Value{};
    }

    if (cast->kind == ImplicitCastKind::Pointer) {
      mlir::Value value = emitExpr(*cast->operand);
      if (!value)
        return {};

      mlir::Type destination = lowerType(*cast->type, cast->loc);
      if (!destination)
        return {};

      if (!mlir::isa<tensorium::PtrType>(value.getType()) ||
          !mlir::isa<tensorium::PtrType>(destination)) {
        diag.error(cast->loc,
                   "MLIR pointer conversion requires object pointers");
        return {};
      }
      if (value.getType() == destination)
        return value;
      return createOp<tensorium::PtrCastOp>(builder, builder.getUnknownLoc(),
                                            destination, value)
          .getResult();
    }
    if (cast->kind == ImplicitCastKind::Arithmetic &&
        cast->operand->type->isIntegral() && cast->type->isIntegral()) {
      return emitIntegerConversion(emitExpr(*cast->operand),
                                   *cast->operand->type, *cast->type,
                                   cast->loc);
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

  if (auto *binary = dynamic_cast<const BinaryExpr *>(&expr))
    return emitBinary(*binary);

  diag.error(expr.loc, "MLIR generation does not support this expression yet");
  return {};
}
