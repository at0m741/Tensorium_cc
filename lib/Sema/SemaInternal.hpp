#pragma once
#include "cc1/AST/AST.hpp"
#include "cc1/AST/Type.hpp"

namespace cc1::sema_detail {
inline bool assignment(BinaryOp op) {
  return op >= BinaryOp::ASSIGN && op <= BinaryOp::RSHIFT_ASSIGN;
}
inline BinaryOp compoundOperation(BinaryOp op) {
  switch (op) {
  case BinaryOp::ADD_ASSIGN:
    return BinaryOp::ADD;
  case BinaryOp::SUB_ASSIGN:
    return BinaryOp::SUB;
  case BinaryOp::MUL_ASSIGN:
    return BinaryOp::MUL;
  case BinaryOp::DIV_ASSIGN:
    return BinaryOp::DIV;
  case BinaryOp::MOD_ASSIGN:
    return BinaryOp::MOD;
  case BinaryOp::AND_ASSIGN:
    return BinaryOp::BITAND;
  case BinaryOp::OR_ASSIGN:
    return BinaryOp::BITOR;
  case BinaryOp::XOR_ASSIGN:
    return BinaryOp::BITXOR;
  case BinaryOp::LSHIFT_ASSIGN:
    return BinaryOp::LSHIFT;
  case BinaryOp::RSHIFT_ASSIGN:
    return BinaryOp::RSHIFT;
  default:
    return op;
  }
}
inline int rank(Type *t) {
  switch (t->kind) {
  case Type::BOOL:
    return 0;
  case Type::CHAR:
  case Type::SCHAR:
  case Type::UCHAR:
    return 1;
  case Type::SHORT:
  case Type::USHORT:
    return 2;
  case Type::INT:
  case Type::UINT:
  case Type::ENUM:
    return 3;
  case Type::LONG:
  case Type::ULONG:
    return 4;
  case Type::LONGLONG:
  case Type::ULONGLONG:
    return 5;
  default:
    return -1;
  }
}
} // namespace cc1::sema_detail
