#include "cc1/AST/AST.hpp"
#include "cc1/AST/ConstantExpr.hpp"
#include "cc1/AST/Type.hpp"
#include "cc1/Basic/TargetInfo.hpp"
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
void expect(bool condition) {
  if (!condition)
    throw std::runtime_error("constant expression boundary check failed");
}
IntLitExpr literal(Type &type, long long value) {
  IntLitExpr expr;
  expr.type = &type;
  expr.val = value;
  return expr;
}
} // namespace

int main() {
  try {
    Type longType(Type::LONG), unsignedType(Type::UINT);
    auto max32 = literal(longType, 2147483647);
    auto one = literal(longType, 1);
    BinaryExpr sum;
    sum.type = &longType;
    sum.op = BinaryOp::ADD;
    sum.lhs = &max32;
    sum.rhs = &one;
    auto result32 = evaluateIntegerConstant(sum, TargetInfo::i386());
    auto result64 = evaluateIntegerConstant(sum, TargetInfo::x86_64());
    expect(!result32.value &&
           result32.error == ConstantEvalError::SignedOverflow);
    expect(result64.value && result64.value->bits == 2147483648ULL &&
           result64.value->bitWidth == 64);

    auto maximum = literal(longType, std::numeric_limits<long long>::max());
    sum.lhs = &maximum;
    auto overflow64 = evaluateIntegerConstant(sum, TargetInfo::x86_64());
    expect(!overflow64.value &&
           overflow64.error == ConstantEvalError::SignedOverflow);

    auto minimum = literal(longType, std::numeric_limits<long long>::min());
    UnaryExpr negate;
    negate.type = &longType;
    negate.op = UnaryOp::NEG;
    negate.operand = &minimum;
    auto negated = evaluateIntegerConstant(negate, TargetInfo::x86_64());
    expect(!negated.value &&
           negated.error == ConstantEvalError::SignedOverflow);

    auto unsignedMax = literal(unsignedType, 4294967295LL);
    auto unsignedOne = literal(unsignedType, 1);
    sum.type = &unsignedType;
    sum.lhs = &unsignedMax;
    sum.rhs = &unsignedOne;
    auto wrapped = evaluateIntegerConstant(sum, TargetInfo::x86_64());
    expect(wrapped.value && wrapped.value->bits == 0 &&
           wrapped.value->isUnsigned);

    Type shortType(Type::SHORT);
    auto negativeShort = literal(shortType, -7);
    ImplicitCastExpr extend;
    extend.type = &longType;
    extend.kind = ImplicitCastKind::Arithmetic;
    extend.operand = &negativeShort;
    auto extended = evaluateIntegerConstant(extend, TargetInfo::x86_64());
    expect(extended.value && extended.value->bits == ~std::uint64_t{6});

    IdentExpr object;
    object.type = &longType;
    expect(!evaluateIntegerConstant(object, TargetInfo::x86_64()).value);
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
