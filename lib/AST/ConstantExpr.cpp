#include "cc1/AST/ConstantExpr.hpp"
#include "cc1/AST/AST.hpp"
#include "cc1/AST/Type.hpp"
#include "cc1/Basic/TargetInfo.hpp"
#include <limits>

namespace {
std::uint64_t mask(unsigned width) {
  return width == 64 ? ~std::uint64_t{0} : (std::uint64_t{1} << width) - 1;
}

std::int64_t signedValue(const IntegerConstant &value) {
  auto bits = value.bits & mask(value.bitWidth);
  if (!(bits & (std::uint64_t{1} << (value.bitWidth - 1))))
    return static_cast<std::int64_t>(bits);
  return -1 - static_cast<std::int64_t>((~bits) & mask(value.bitWidth));
}
} // namespace

IntegerConstantResult evaluateIntegerConstant(const Expr &expr,
                                              const TargetInfo &target) {
  if (!expr.type || !expr.type->isIntegral())
    return {{}, ConstantEvalError::Unsupported};
  unsigned width = 0;
  switch (expr.type->kind) {
  case Type::BOOL:
    width = 1;
    break;
  case Type::CHAR:
  case Type::SCHAR:
  case Type::UCHAR:
    width = 8;
    break;
  case Type::SHORT:
  case Type::USHORT:
    width = target.sizeofShort * 8;
    break;
  case Type::INT:
  case Type::UINT:
    width = target.sizeofInt * 8;
    break;
  case Type::LONG:
  case Type::ULONG:
    width = target.sizeofLong * 8;
    break;
  case Type::LONGLONG:
  case Type::ULONGLONG:
    width = target.sizeofLonglong * 8;
    break;
  default:
    return {{}, ConstantEvalError::Unsupported};
  }
  if (!width || width > 64)
    return {{}, ConstantEvalError::Unsupported};
  const bool isUnsigned = !expr.type->isSigned();
  auto result = [&](std::uint64_t bits) -> IntegerConstantResult {
    return {IntegerConstant{bits & mask(width), width, isUnsigned},
            ConstantEvalError::None};
  };
  if (auto *literal = dynamic_cast<const IntLitExpr *>(&expr))
    return result(static_cast<std::uint64_t>(literal->val));
  if (auto *cast = dynamic_cast<const ImplicitCastExpr *>(&expr)) {
    if (cast->kind != ImplicitCastKind::Arithmetic)
      return {{}, ConstantEvalError::NotConstant};
    auto operand = evaluateIntegerConstant(*cast->operand, target);
    if (!operand.value)
      return operand;
    auto value = *operand.value;
    if (expr.type->kind == Type::BOOL)
      return result(value.bits != 0);
    return result(value.isUnsigned
                      ? value.bits
                      : static_cast<std::uint64_t>(signedValue(value)));
  }
  const auto min = width == 64 ? std::numeric_limits<std::int64_t>::min()
                               : -(std::int64_t{1} << (width - 1));
  const auto max = width == 64 ? std::numeric_limits<std::int64_t>::max()
                               : (std::int64_t{1} << (width - 1)) - 1;
  if (auto *unary = dynamic_cast<const UnaryExpr *>(&expr)) {
    if (unary->op != UnaryOp::POS && unary->op != UnaryOp::NEG)
      return {{}, ConstantEvalError::Unsupported};
    auto operand = evaluateIntegerConstant(*unary->operand, target);
    if (!operand.value)
      return operand;
    auto value = *operand.value;
    if (unary->op == UnaryOp::POS)
      return result(value.bits);
    if (!isUnsigned && signedValue(value) == min)
      return {{}, ConstantEvalError::SignedOverflow};
    return result(std::uint64_t{0} - value.bits);
  }
  if (auto *binary = dynamic_cast<const BinaryExpr *>(&expr)) {
    if (binary->op != BinaryOp::ADD && binary->op != BinaryOp::SUB &&
        binary->op != BinaryOp::MUL)
      return {{}, ConstantEvalError::Unsupported};
    auto lhs = evaluateIntegerConstant(*binary->lhs, target);
    auto rhs = evaluateIntegerConstant(*binary->rhs, target);
    if (!lhs.value)
      return lhs;
    if (!rhs.value)
      return rhs;
    if (isUnsigned) {
      auto a = lhs.value->bits, b = rhs.value->bits;
      return result(binary->op == BinaryOp::ADD   ? a + b
                    : binary->op == BinaryOp::SUB ? a - b
                                                  : a * b);
    }
    auto a = signedValue(*lhs.value), b = signedValue(*rhs.value);
    std::int64_t value;
    bool overflow =
        binary->op == BinaryOp::ADD   ? __builtin_add_overflow(a, b, &value)
        : binary->op == BinaryOp::SUB ? __builtin_sub_overflow(a, b, &value)
                                      : __builtin_mul_overflow(a, b, &value);
    if (overflow || value < min || value > max)
      return {{}, ConstantEvalError::SignedOverflow};
    return result(static_cast<std::uint64_t>(value));
  }
  return {{}, ConstantEvalError::NotConstant};
}
