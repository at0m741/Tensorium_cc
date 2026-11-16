#include "cc1/CodeGen/MLIRGen.hpp"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "tensorium/Dialect/Tensorium/IR/TensoriumTypes.h"
#include "tensorium/Support/Builders.h"

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

  case Type::POINTER: {
    if (!type.pointee ||
        !(type.pointee->isArithmetic() || type.pointee->isPointer() ||
          type.pointee->isArray())) {
      diag.error(loc, "MLIR generation currently supports only pointers "
                      "to supported object types");
      return {};
    }
    if (type.pointee->quals.isVolatile) {
      diag.error(loc, "MLIR generation does not support volatile pointees yet");
      return {};
    }

    mlir::Type pointee = lowerType(*type.pointee, loc);
    if (!pointee)
      return {};

    return tensorium::PtrType::get(builder.getContext(), pointee);
  }
  case Type::ARRAY: {
    if (!type.elemType || type.arraySize <= 0) {
      diag.error(loc, "MLIR generation requires a complete fixed-size array");
      return {};
    }
    if (type.elemType->quals.isVolatile) {
      diag.error(
          loc, "MLIR generation does not support volatile array elements yet");
      return {};
    }
    mlir::Type element = lowerType(*type.elemType, loc);
    if (!element)
      return {};
    return tensorium::ArrayType::get(builder.getContext(), type.arraySize,
                                     element);
  }
  default:
    diag.error(loc, "MLIR generation does not support type '" + type.str() +
                        "' yet");
    return {};
  }
}

mlir::Value MLIRGen::emitIntegerConversion(mlir::Value value, const Type &from,
                                           const Type &to,
                                           SourceLoc sourceLoc) {
  using tensorium::createOp;
  if (!value)
    return {};
  auto destination = lowerType(to, sourceLoc);
  if (!destination)
    return {};
  auto source = mlir::dyn_cast<mlir::IntegerType>(value.getType());
  auto result = mlir::dyn_cast<mlir::IntegerType>(destination);
  if (!source || !result) {
    diag.error(sourceLoc, "integer conversion requires integer MLIR types");
    return {};
  }
  auto loc = builder.getUnknownLoc();
  if (to.kind == Type::BOOL) {
    auto zero = createOp<mlir::arith::ConstantOp>(
        builder, loc, builder.getIntegerAttr(source, 0));
    return createOp<mlir::arith::CmpIOp>(builder, loc,
                                         mlir::arith::CmpIPredicate::ne, value,
                                         zero.getResult())
        .getResult();
  }
  if (source == result)
    return value;
  if (source.getWidth() > result.getWidth())
    return createOp<mlir::arith::TruncIOp>(builder, loc, result, value)
        .getResult();
  return from.isSigned()
             ? createOp<mlir::arith::ExtSIOp>(builder, loc, result, value)
                   .getResult()
             : createOp<mlir::arith::ExtUIOp>(builder, loc, result, value)
                   .getResult();
}
