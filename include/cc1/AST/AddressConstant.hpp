#pragma once

#include <cstdint>
#include <optional>
#include <vector>

struct Expr;
struct Type;
struct VarDecl;
struct TargetInfo;

struct AddressConstant {
  const VarDecl *object;
  const Type *pointeeType;
  // Typed GEP path: object offset, then one index per array dimension.
  // An empty path denotes the start of the complete object.
  std::vector<std::int64_t> indices;
};

enum class AddressConstantError {
  None,
  NotConstant,
  InvalidOffset,
  OutOfBounds
};

struct AddressConstantResult {
  std::optional<AddressConstant> value;
  AddressConstantError error = AddressConstantError::NotConstant;
  const Expr *errorExpr = nullptr;
};

// Requires a typed AST. Pointer-valued object reads are not address constants.
AddressConstantResult evaluateAddressConstant(const Expr &expr,
                                              const TargetInfo &target);
