#include "cc1/AST/AddressConstant.hpp"
#include "cc1/AST/AST.hpp"
#include "cc1/AST/ConstantExpr.hpp"
#include "cc1/AST/Type.hpp"
#include <limits>
#include <utility>

namespace {
struct State {
  AddressConstant address;
  std::int64_t position = 0;
  std::int64_t extent = 1;
};

class Evaluator {
public:
  explicit Evaluator(const TargetInfo &target) : target(target) {}

  AddressConstantResult evaluate(const Expr &expr) {
    auto state = pointer(expr);
    if (!state)
      return {{}, error, errorExpr};
    return {std::move(state->address), AddressConstantError::None, nullptr};
  }

private:
  const TargetInfo &target;
  AddressConstantError error = AddressConstantError::NotConstant;
  const Expr *errorExpr = nullptr;

  std::optional<State> fail(const Expr &expr, AddressConstantError reason) {
    error = reason;
    errorExpr = &expr;
    return {};
  }

  std::optional<State> offset(State state, const Expr &expr, bool subtract) {
    auto constant = evaluateIntegerConstant(expr, target);
    if (!constant.value)
      return fail(expr, AddressConstantError::InvalidOffset);
    auto value = *constant.value;
    std::int64_t displacement;
    if (!value.isUnsigned &&
        (value.bits & (std::uint64_t{1} << (value.bitWidth - 1)))) {
      auto mask = value.bitWidth == 64
                      ? ~std::uint64_t{0}
                      : (std::uint64_t{1} << value.bitWidth) - 1;
      displacement = -1 - static_cast<std::int64_t>((~value.bits) & mask);
    } else {
      if (value.bits >
          static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()))
        return fail(expr, AddressConstantError::OutOfBounds);
      displacement = static_cast<std::int64_t>(value.bits);
    }
    std::int64_t position;
    bool overflow =
        subtract
            ? __builtin_sub_overflow(state.position, displacement, &position)
            : __builtin_add_overflow(state.position, displacement, &position);
    if (overflow || position < 0 || position > state.extent)
      return fail(expr, AddressConstantError::OutOfBounds);
    state.position = position;
    if (state.address.indices.empty())
      state.address.indices.push_back(position);
    else
      state.address.indices.back() = position;
    return state;
  }

  std::optional<State> lvalue(const Expr &expr) {
    if (auto *reference = dynamic_cast<const IdentExpr *>(&expr)) {
      auto *object = dynamic_cast<const VarDecl *>(reference->decl);
      if (object && object->isGlobal && object->type)
        return State{AddressConstant{object, object->type, {}}};
    }
    if (auto *index = dynamic_cast<const IndexExpr *>(&expr)) {
      const Expr *base = index->base;
      const Expr *integer = index->index;
      if (base->type->isIntegral())
        std::swap(base, integer);
      auto state = pointer(*base);
      return state ? offset(std::move(*state), *integer, false) : std::nullopt;
    }
    if (auto *unary = dynamic_cast<const UnaryExpr *>(&expr))
      if (unary->op == UnaryOp::DEREF)
        return pointer(*unary->operand);
    return fail(expr, AddressConstantError::NotConstant);
  }

  std::optional<State> pointer(const Expr &expr) {
    if (auto *cast = dynamic_cast<const ImplicitCastExpr *>(&expr)) {
      if (cast->kind == ImplicitCastKind::Pointer)
        return pointer(*cast->operand);
      if (cast->kind == ImplicitCastKind::ArrayToPointer) {
        auto state = lvalue(*cast->operand);
        if (!state)
          return {};
        const Type *array = state->address.pointeeType;
        // A one-past address can be formed, but cannot be used to access an
        // inner array. This also prevents flattening across row boundaries.
        if (!array->isArray() || array->arraySize <= 0 ||
            state->position == state->extent)
          return fail(expr, AddressConstantError::OutOfBounds);
        if (state->address.indices.empty())
          state->address.indices.push_back(0);
        state->address.indices.push_back(0);
        state->address.pointeeType = array->elemType;
        state->position = 0;
        state->extent = array->arraySize;
        return state;
      }
    }
    if (auto *unary = dynamic_cast<const UnaryExpr *>(&expr))
      if (unary->op == UnaryOp::ADDR)
        return lvalue(*unary->operand);
    if (auto *binary = dynamic_cast<const BinaryExpr *>(&expr)) {
      if (binary->op == BinaryOp::ADD || binary->op == BinaryOp::SUB) {
        const Expr *base = binary->lhs;
        const Expr *integer = binary->rhs;
        if (binary->op == BinaryOp::ADD && base->type->isIntegral())
          std::swap(base, integer);
        if (!base->type->isPointer() || !integer->type->isIntegral())
          return fail(expr, AddressConstantError::NotConstant);
        auto state = pointer(*base);
        return state ? offset(std::move(*state), *integer,
                              binary->op == BinaryOp::SUB)
                     : std::nullopt;
      }
    }
    return fail(expr, AddressConstantError::NotConstant);
  }
};
} // namespace

AddressConstantResult evaluateAddressConstant(const Expr &expr,
                                              const TargetInfo &target) {
  return Evaluator(target).evaluate(expr);
}
