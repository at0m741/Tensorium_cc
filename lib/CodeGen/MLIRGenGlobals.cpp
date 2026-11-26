#include "cc1/AST/AddressConstant.hpp"
#include "cc1/AST/ConstantExpr.hpp"
#include "cc1/CodeGen/MLIRGen.hpp"
#include "tensorium/Dialect/Tensorium/IR/TensoriumOps.h"
#include "tensorium/Support/Builders.h"
#include "llvm/ADT/SmallVector.h"

mlir::LogicalResult MLIRGen::emitGlobals(const TranslationUnit &unit) {
  llvm::SmallVector<const Decl *> order;
  for (const Decl *decl : unit.decls) {
    auto *var = dynamic_cast<const VarDecl *>(decl);
    if (!var)
      continue;
    const Decl *key = var->canonicalDecl ? var->canonicalDecl : var;
    auto previous = globals.find(key);
    if (previous == globals.end()) {
      globals[key] = var;
      order.push_back(key);
    } else if (var->init ||
               (!previous->second->init && var->sc != StorageClass::EXTERN)) {
      globals[key] = var;
    }
  }
  for (const Decl *key : order) {
    const auto &var = *globals.lookup(key);
    if (var.type->quals.isVolatile) {
      diag.error(var.loc,
                 "MLIR generation does not support volatile globals yet");
      return mlir::failure();
    }
    auto type = lowerType(*var.type, var.loc);
    if (!type)
      return mlir::failure();
    mlir::Attribute initial;
    if (var.init) {
      auto *cast = dynamic_cast<const ImplicitCastExpr *>(var.init);
      if (var.type->isPointer() && cast &&
          cast->kind == ImplicitCastKind::NullToPointer) {
        initial = builder.getUnitAttr();
      } else if (var.type->isPointer()) {
        auto address = evaluateAddressConstant(*var.init, target);
        if (!address.value) {
          const char *message =
              "global pointer initializer requires a global object address "
              "or a null pointer constant";
          if (address.error == AddressConstantError::InvalidOffset)
            message = "global address offset requires a supported, "
                      "non-overflowing integer constant expression";
          else if (address.error == AddressConstantError::OutOfBounds)
            message =
                "global address offset is outside the object or array bounds";
          diag.error(address.errorExpr ? address.errorExpr->loc : var.init->loc,
                     message);
          return mlir::failure();
        }
        const auto &constant = *address.value;
        const Decl *key = constant.object->canonicalDecl
                              ? constant.object->canonicalDecl
                              : constant.object;
        auto referenced = globals.find(key);
        if (referenced == globals.end()) {
          diag.error(var.init->loc,
                     "unresolved object in global address initializer");
          return mlir::failure();
        }
        auto pointee = lowerType(*constant.pointeeType, var.init->loc);
        if (!pointee)
          return mlir::failure();
        auto destination =
            mlir::cast<tensorium::PtrType>(type).getPointeeType();
        if (destination != pointee &&
            !mlir::isa<tensorium::VoidType>(destination)) {
          diag.error(
              var.init->loc,
              "global address initializer requires matching object types");
          return mlir::failure();
        }
        initial = tensorium::GlobalAddressAttr::get(
            builder.getContext(),
            mlir::FlatSymbolRefAttr::get(builder.getContext(),
                                         referenced->second->name),
            constant.indices);
      } else if (!var.type->isIntegral()) {
        diag.error(
            var.loc,
            "MLIR global initializers currently require an integer object");
        return mlir::failure();
      } else {
        auto value = evaluateIntegerConstant(*var.init, target);
        if (!value.value) {
          diag.error(var.init->loc,
                     value.error == ConstantEvalError::SignedOverflow
                         ? "signed overflow in global initializer"
                         : "global initializer requires a supported integer "
                           "constant expression");
          return mlir::failure();
        }
        initial = builder.getIntegerAttr(
            type, llvm::APInt(value.value->bitWidth, value.value->bits));
      }
    } else if (var.sc != StorageClass::EXTERN) {
      initial = builder.getUnitAttr();
    }
    const bool internal =
        static_cast<const VarDecl *>(key)->sc == StorageClass::STATIC;
    auto global = tensorium::createOp<tensorium::GlobalOp>(
        builder, builder.getUnknownLoc(), builder.getStringAttr(var.name),
        mlir::TypeAttr::get(type),
        tensorium::GlobalLinkageAttr::get(
            builder.getContext(), internal
                                      ? tensorium::GlobalLinkage::internal
                                      : tensorium::GlobalLinkage::external),
        initial,
        var.type->quals.isConst ? builder.getUnitAttr() : mlir::UnitAttr{});
    // MLIR declarations are private symbols; native linkage stays external.
    if (!initial)
      global.setVisibility(mlir::SymbolTable::Visibility::Private);
  }
  return mlir::success();
}
