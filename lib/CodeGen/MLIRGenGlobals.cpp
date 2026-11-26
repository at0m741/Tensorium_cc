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
        const Expr *expression = var.init;
        // Compatible pointer conversions can add pointee qualifiers.
        while (auto *conversion =
                   dynamic_cast<const ImplicitCastExpr *>(expression)) {
          if (conversion->kind != ImplicitCastKind::Pointer)
            break;
          expression = conversion->operand;
        }
        auto *address = dynamic_cast<const UnaryExpr *>(expression);
        auto *reference =
            address && address->op == UnaryOp::ADDR
                ? dynamic_cast<const IdentExpr *>(address->operand)
                : nullptr;
        const Decl *decl = reference ? reference->decl : nullptr;
        const Decl *referencedKey =
            decl && decl->canonicalDecl ? decl->canonicalDecl : decl;
        auto referenced = globals.find(referencedKey);
        if (referenced == globals.end()) {
          diag.error(
              var.init->loc,
              "global pointer initializer requires a global object address "
              "or a null pointer constant");
          return mlir::failure();
        }
        auto object = lowerType(*referenced->second->type, var.init->loc);
        if (!object)
          return mlir::failure();
        if (mlir::cast<tensorium::PtrType>(type).getPointeeType() != object) {
          diag.error(
              var.init->loc,
              "global address initializer requires matching object types");
          return mlir::failure();
        }
        initial = tensorium::GlobalAddressAttr::get(
            builder.getContext(),
            mlir::FlatSymbolRefAttr::get(builder.getContext(),
                                         referenced->second->name));
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
