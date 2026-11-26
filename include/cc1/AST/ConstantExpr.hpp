#pragma once
#include <cstdint>
#include <optional>

struct Expr;
struct TargetInfo;

struct IntegerConstant {
  std::uint64_t bits;
  unsigned bitWidth;
  bool isUnsigned;
};

enum class ConstantEvalError { None, NotConstant, Unsupported, SignedOverflow };

struct IntegerConstantResult {
  std::optional<IntegerConstant> value;
  ConstantEvalError error = ConstantEvalError::NotConstant;
};

// Evaluate a typed AST using target widths, independently of MLIR.
// Initially supports integer literals, arithmetic conversions and unary/binary
// +, - and *. Unsupported expressions never silently become zero.
IntegerConstantResult evaluateIntegerConstant(const Expr &expr,
                                              const TargetInfo &target);
