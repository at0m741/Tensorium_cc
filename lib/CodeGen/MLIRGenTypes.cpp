#include "cc1/CodeGen/MLIRGen.hpp"
#include "tensorium/Dialect/Tensorium/IR/TensoriumTypes.h"

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
    if (!type.pointee || !type.pointee->isArithmetic()) {
      diag.error(loc, "MLIR generation currently support only pointers "
                      "to arithmetic objects");
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
  default:
    diag.error(loc, "MLIR generation does not support type '" + type.str() +
                        "' yet");
    return {};
  }
}
