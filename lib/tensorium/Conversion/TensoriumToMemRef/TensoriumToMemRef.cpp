#include "tensorium/Conversion/TensoriumToMemRef/TensoriumToMemRef.h"
#include "tensorium/Dialect/Tensorium/IR/TensoriumDialect.h"
#include "tensorium/Dialect/Tensorium/IR/TensoriumOps.h"

#include "mlir/Dialect/ControlFlow/IR/ControlFlowOps.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/Func/Transforms/FuncConversions.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassRegistry.h"
#include "mlir/Transforms/DialectConversion.h"
#include "llvm/Config/llvm-config.h"

using namespace mlir;

namespace {

bool containsTensoriumType(Type type) {
  bool found = false;
  type.walk([&](Type nested) {
    found |= nested.getDialect().getNamespace() == "tensorium";
  });
  return found;
}

struct AllocaLowering : OpConversionPattern<tensorium::AllocaOp> {
  using OpConversionPattern::OpConversionPattern;

  LogicalResult
  matchAndRewrite(tensorium::AllocaOp op, OpAdaptor,
                  ConversionPatternRewriter &rewriter) const override {
    auto type = dyn_cast_or_null<MemRefType>(
        getTypeConverter()->convertType(op.getResult().getType()));
    if (!type)
      return failure();
#if LLVM_VERSION_MAJOR >= 22
    auto allocation = memref::AllocaOp::create(rewriter, op.getLoc(), type);
#else
    auto allocation = rewriter.create<memref::AllocaOp>(op.getLoc(), type);
#endif
    rewriter.replaceOp(op, allocation.getResult());
    return success();
  }
};

struct LoadLowering : OpConversionPattern<tensorium::LoadOp> {
  using OpConversionPattern::OpConversionPattern;

  LogicalResult
  matchAndRewrite(tensorium::LoadOp op, OpAdaptor adaptor,
                  ConversionPatternRewriter &rewriter) const override {
#if LLVM_VERSION_MAJOR >= 22
    auto load = memref::LoadOp::create(rewriter, op.getLoc(),
                                       adaptor.getAddress(), ValueRange{});
#else
    auto load = rewriter.create<memref::LoadOp>(
        op.getLoc(), adaptor.getAddress(), ValueRange{});
#endif
    rewriter.replaceOp(op, load.getResult());
    return success();
  }
};

struct StoreLowering : OpConversionPattern<tensorium::StoreOp> {
  using OpConversionPattern::OpConversionPattern;

  LogicalResult
  matchAndRewrite(tensorium::StoreOp op, OpAdaptor adaptor,
                  ConversionPatternRewriter &rewriter) const override {
#if LLVM_VERSION_MAJOR >= 22
    memref::StoreOp::create(rewriter, op.getLoc(), adaptor.getValue(),
                            adaptor.getAddress(), ValueRange{});
#else
    rewriter.create<memref::StoreOp>(op.getLoc(), adaptor.getValue(),
                                     adaptor.getAddress(), ValueRange{});
#endif
    rewriter.eraseOp(op);
    return success();
  }
};

struct FunctionLowering : OpConversionPattern<func::FuncOp> {
  using OpConversionPattern::OpConversionPattern;

  LogicalResult
  matchAndRewrite(func::FuncOp function, OpAdaptor,
                  ConversionPatternRewriter &rewriter) const override {
    FunctionType type = function.getFunctionType();
    const TypeConverter &converter = *getTypeConverter();
    TypeConverter::SignatureConversion signature(type.getNumInputs());
    SmallVector<Type> results;
    if (failed(converter.convertSignatureArgs(type.getInputs(), signature)) ||
        failed(converter.convertTypes(type.getResults(), results)))
      return failure();

    // Convert every block, even when the function's public signature contains
    // only scalars. Pointer block arguments can still occur in its CFG.
    if (failed(rewriter.convertRegionTypes(&function.getBody(), converter,
                                           &signature)))
      return failure();
    auto converted = FunctionType::get(rewriter.getContext(),
                                       signature.getConvertedTypes(), results);
    rewriter.modifyOpInPlace(function, [&] { function.setType(converted); });
    return success();
  }
};

struct TensoriumToMemRefPass
    : PassWrapper<TensoriumToMemRefPass, OperationPass<ModuleOp>> {
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(TensoriumToMemRefPass)

  StringRef getArgument() const final { return "convert-tensorium-to-memref"; }
  StringRef getDescription() const final {
    return "Lower Tensorium scalar storage to rank-zero memrefs";
  }

  void getDependentDialects(DialectRegistry &registry) const override {
    registry.insert<memref::MemRefDialect, func::FuncDialect,
                    cf::ControlFlowDialect>();
  }

  void runOnOperation() override {
    // Fail before rewriting rather than inventing a representation for nested
    // pointers or silently changing the ABI of external declarations.
    WalkResult validation =
        getOperation().walk([&](Operation *op) -> WalkResult {
          if (auto function = dyn_cast<func::FuncOp>(op)) {
            if (function.isExternal() &&
                containsTensoriumType(function.getFunctionType())) {
              function.emitOpError("external Tensorium pointer ABI requires an "
                                   "explicit boundary conversion");
              return WalkResult::interrupt();
            }
          }

          bool unsupported = false;
          auto check = [&](Type type) {
            type.walk([&](tensorium::PtrType pointer) {
              unsupported |=
                  !isa<IntegerType, FloatType>(pointer.getPointeeType());
            });
          };
          for (Type type : op->getOperandTypes())
            check(type);
          for (Type type : op->getResultTypes())
            check(type);
          for (Region &region : op->getRegions())
            for (Block &block : region)
              for (Type type : block.getArgumentTypes())
                check(type);
          if (auto function = dyn_cast<func::FuncOp>(op))
            check(function.getFunctionType());
          if (unsupported) {
            op->emitOpError(
                "Tensorium-to-MemRef only supports scalar pointee types "
                "(integer or floating-point)");
            return WalkResult::interrupt();
          }
          return WalkResult::advance();
        });
    if (validation.wasInterrupted()) {
      signalPassFailure();
      return;
    }

    TypeConverter converter;
    converter.addConversion([](Type type) -> Type {
      return containsTensoriumType(type) ? Type{} : type;
    });
    converter.addConversion([](tensorium::PtrType pointer) -> Type {
      Type element = pointer.getPointeeType();
      if (!isa<IntegerType, FloatType>(element))
        return {};
      return MemRefType::get({}, element);
    });

    RewritePatternSet patterns(&getContext());
    patterns.add<AllocaLowering, LoadLowering, StoreLowering, FunctionLowering>(
        converter, &getContext());
    populateCallOpTypeConversionPattern(patterns, converter);
    populateReturnOpTypeConversionPattern(patterns, converter);
    populateBranchOpInterfaceTypeConversionPattern(patterns, converter);

    ConversionTarget target(getContext());
    target.addIllegalDialect<tensorium::TensoriumDialect>();
    target.addIllegalOp<UnrealizedConversionCastOp>();
    target.addDynamicallyLegalOp<func::FuncOp>([&](func::FuncOp function) {
      return converter.isSignatureLegal(function.getFunctionType()) &&
             converter.isLegal(&function.getBody());
    });
    target.markUnknownOpDynamicallyLegal([&](Operation *op) {
      if (!converter.isLegal(op))
        return false;
      for (Region &region : op->getRegions())
        if (!converter.isLegal(&region))
          return false;
      bool attributesLegal = true;
      op->getAttrDictionary().walk(
          [&](Type type) { attributesLegal &= !containsTensoriumType(type); });
      return attributesLegal;
    });

    if (failed(
            applyFullConversion(getOperation(), target, std::move(patterns))))
      signalPassFailure();
  }
};

} // namespace

std::unique_ptr<Pass> tensorium::createTensoriumToMemRefPass() {
  return std::make_unique<TensoriumToMemRefPass>();
}

void tensorium::registerTensoriumToMemRefPass() {
  static PassRegistration<TensoriumToMemRefPass> registration;
}
