#include "tensorium/Conversion/TensoriumToLLVM/TensoriumToLLVM.h"
#include "mlir/Conversion/ArithToLLVM/ArithToLLVM.h"
#include "mlir/Conversion/ControlFlowToLLVM/ControlFlowToLLVM.h"
#include "mlir/Conversion/FuncToLLVM/ConvertFuncToLLVM.h"
#include "mlir/Conversion/LLVMCommon/ConversionTarget.h"
#include "mlir/Conversion/LLVMCommon/LoweringOptions.h"
#include "mlir/Conversion/LLVMCommon/TypeConverter.h"
#include "mlir/Dialect/LLVMIR/LLVMDialect.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassRegistry.h"
#include "tensorium/Dialect/Tensorium/IR/TensoriumDialect.h"
#include "tensorium/Dialect/Tensorium/IR/TensoriumOps.h"
#include "tensorium/Support/Builders.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/Support/Error.h"
#include <type_traits>

using namespace mlir;
using tensorium::createOp;

namespace {

class ObjectTypeConverter : public LLVMTypeConverter {
public:
  ObjectTypeConverter(MLIRContext *context, const LowerToLLVMOptions &options)
      : LLVMTypeConverter(context, options) {
    addConversion([context](tensorium::PtrType) -> Type {
      return LLVM::LLVMPointerType::get(context);
    });
    addConversion([this](tensorium::ArrayType array) -> Type {
      Type element = convertType(array.getElementType());
      return element ? LLVM::LLVMArrayType::get(element, array.getSize())
                     : Type{};
    });
  }
};

template <typename Op> struct MemoryLowering : OpConversionPattern<Op> {
  using OpConversionPattern<Op>::OpConversionPattern;
  using Adaptor = typename Op::Adaptor;

  LogicalResult
  matchAndRewrite(Op op, Adaptor adaptor,
                  ConversionPatternRewriter &rewriter) const override {
    auto loc = op.getLoc();
    const auto *converter = this->getTypeConverter();
    if constexpr (std::is_same_v<Op, tensorium::AllocaOp>) {
      Type element =
          converter->convertType(op.getResult().getType().getPointeeType());
      if (!element)
        return rewriter.notifyMatchFailure(op,
                                           "unsupported allocated object type");
      auto integer =
          static_cast<const LLVMTypeConverter *>(converter)->getIndexType();
      auto one = createOp<LLVM::ConstantOp>(
          rewriter, loc, integer, rewriter.getIntegerAttr(integer, 1));
      auto allocation = createOp<LLVM::AllocaOp>(
          rewriter, loc, converter->convertType(op.getResult().getType()),
          element, one.getResult());
      rewriter.replaceOp(op, allocation.getResult());
    } else if constexpr (std::is_same_v<Op, tensorium::GlobalOp>) {
      Type type = converter->convertType(op.getObjectType());
      if (!type)
        return rewriter.notifyMatchFailure(op,
                                           "unsupported global object type");
      Attribute value;
      bool zero = false;
      tensorium::GlobalAddressAttr address;
      if (auto initial = op.getInitialValue()) {
        zero = isa<UnitAttr>(*initial);
        address = dyn_cast<tensorium::GlobalAddressAttr>(*initial);
        if (!zero && !address)
          value = *initial;
      }
      auto global = createOp<LLVM::GlobalOp>(
          rewriter, loc, type, op.getConstant(),
          op.getLinkage() == tensorium::GlobalLinkage::internal
              ? LLVM::Linkage::Internal
              : LLVM::Linkage::External,
          op.getSymName(), value);
      if (zero || address) {
        OpBuilder::InsertionGuard guard(rewriter);
        auto *block = new Block();
        global.getInitializerRegion().push_back(block);
        rewriter.setInsertionPointToStart(block);
        Value initial =
            address ? createOp<LLVM::AddressOfOp>(rewriter, loc, type,
                                                  address.getGlobal())
                          .getResult()
                    : createOp<LLVM::ZeroOp>(rewriter, loc, type).getResult();
        createOp<LLVM::ReturnOp>(rewriter, loc, initial);
      }
      rewriter.eraseOp(op);
    } else if constexpr (std::is_same_v<Op, tensorium::GetGlobalOp>) {
      auto address = createOp<LLVM::AddressOfOp>(
          rewriter, loc, converter->convertType(op.getResult().getType()),
          op.getNameAttr());
      rewriter.replaceOp(op, address.getResult());
    } else if constexpr (std::is_same_v<Op, tensorium::PtrCastOp>) {
      // Tensorium object and void pointers share LLVM's opaque pointer type.
      rewriter.replaceOp(op, adaptor.getSource());
    } else if constexpr (std::is_same_v<Op, tensorium::LoadOp>) {
      auto load = createOp<LLVM::LoadOp>(
          rewriter, loc, converter->convertType(op.getResult().getType()),
          adaptor.getAddress());
      rewriter.replaceOp(op, load.getResult());
    } else if constexpr (std::is_same_v<Op, tensorium::StoreOp>) {
      createOp<LLVM::StoreOp>(rewriter, loc, adaptor.getValue(),
                              adaptor.getAddress());
      rewriter.eraseOp(op);
    } else if constexpr (std::is_same_v<Op, tensorium::NullOp>) {
      auto zero = createOp<LLVM::ZeroOp>(
          rewriter, loc, converter->convertType(op.getResult().getType()));
      rewriter.replaceOp(op, zero.getResult());
    } else if constexpr (std::is_same_v<Op, tensorium::PtrOffsetOp>) {
      Type element =
          converter->convertType(op.getBase().getType().getPointeeType());
      SmallVector<LLVM::GEPArg> indices{adaptor.getOffset()};
      auto offset =
          createOp<LLVM::GEPOp>(rewriter, loc, adaptor.getBase().getType(),
                                element, adaptor.getBase(), indices);
      rewriter.replaceOp(op, offset.getResult());
    } else if constexpr (std::is_same_v<Op, tensorium::ArrayDecayOp>) {
      Type array =
          converter->convertType(op.getBase().getType().getPointeeType());
      SmallVector<LLVM::GEPArg> indices{0, 0};
      auto decay =
          createOp<LLVM::GEPOp>(rewriter, loc, adaptor.getBase().getType(),
                                array, adaptor.getBase(), indices);
      rewriter.replaceOp(op, decay.getResult());
    } else if constexpr (std::is_same_v<Op, tensorium::PtrCmpOp>) {
      LLVM::ICmpPredicate predicate;
      switch (op.getPredicate()) {
      case tensorium::PtrPredicate::eq:
        predicate = LLVM::ICmpPredicate::eq;
        break;
      case tensorium::PtrPredicate::ne:
        predicate = LLVM::ICmpPredicate::ne;
        break;
      case tensorium::PtrPredicate::ult:
        predicate = LLVM::ICmpPredicate::ult;
        break;
      case tensorium::PtrPredicate::ule:
        predicate = LLVM::ICmpPredicate::ule;
        break;
      case tensorium::PtrPredicate::ugt:
        predicate = LLVM::ICmpPredicate::ugt;
        break;
      case tensorium::PtrPredicate::uge:
        predicate = LLVM::ICmpPredicate::uge;
        break;
      }
      auto comparison = createOp<LLVM::ICmpOp>(
          rewriter, loc, predicate, adaptor.getLhs(), adaptor.getRhs());
      rewriter.replaceOp(op, comparison.getResult());
    } else if constexpr (std::is_same_v<Op, tensorium::PtrDiffOp>) {
      // GEP index width can differ from address width in a data layout.
      // Subtract full pointer representations before dividing by the stride.
      Type integer = rewriter.getIntegerType(
          static_cast<const LLVMTypeConverter *>(converter)
              ->getPointerBitwidth());
      Type element =
          converter->convertType(op.getLhs().getType().getPointeeType());
      auto lhs =
          createOp<LLVM::PtrToIntOp>(rewriter, loc, integer, adaptor.getLhs());
      auto rhs =
          createOp<LLVM::PtrToIntOp>(rewriter, loc, integer, adaptor.getRhs());
      auto bytes = createOp<LLVM::SubOp>(rewriter, loc, lhs.getResult(),
                                         rhs.getResult());
      auto zero =
          createOp<LLVM::ZeroOp>(rewriter, loc, adaptor.getLhs().getType());
      SmallVector<LLVM::GEPArg> indices{1};
      auto next =
          createOp<LLVM::GEPOp>(rewriter, loc, zero.getResult().getType(),
                                element, zero.getResult(), indices);
      auto size =
          createOp<LLVM::PtrToIntOp>(rewriter, loc, integer, next.getResult());
      Value result = createOp<LLVM::SDivOp>(rewriter, loc, bytes.getResult(),
                                            size.getResult())
                         .getResult();
      auto destination =
          cast<IntegerType>(converter->convertType(op.getResult().getType()));
      if (destination.getWidth() < cast<IntegerType>(integer).getWidth())
        result = createOp<LLVM::TruncOp>(rewriter, loc, destination, result)
                     .getResult();
      else if (destination != integer)
        result = createOp<LLVM::SExtOp>(rewriter, loc, destination, result)
                     .getResult();
      rewriter.replaceOp(op, result);
    }
    return success();
  }
};

struct TensoriumToLLVMPass
    : PassWrapper<TensoriumToLLVMPass, OperationPass<ModuleOp>> {
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(TensoriumToLLVMPass)
  StringRef getArgument() const final { return "convert-tensorium-to-llvm"; }
  StringRef getDescription() const final {
    return "Lower Tensorium object memory and arith/cf/func to native LLVM IR";
  }
  void getDependentDialects(DialectRegistry &registry) const override {
    registry.insert<LLVM::LLVMDialect>();
  }
  void runOnOperation() override {
    auto layout = getOperation()->getAttrOfType<StringAttr>("llvm.data_layout");
    if (!layout) {
      getOperation().emitError(
          "Tensorium-to-LLVM requires an explicit llvm.data_layout");
      signalPassFailure();
      return;
    }
    auto parsed = llvm::DataLayout::parse(layout.getValue());
    if (!parsed) {
      getOperation().emitError("invalid LLVM data layout: ")
          << llvm::toString(parsed.takeError());
      signalPassFailure();
      return;
    }
    LowerToLLVMOptions options(&getContext());
    options.dataLayout = *parsed;
    options.overrideIndexBitwidth(parsed->getIndexSizeInBits(0));
    ObjectTypeConverter converter(&getContext(), options);
    RewritePatternSet patterns(&getContext());
    patterns.add<
        MemoryLowering<tensorium::GlobalOp>,
        MemoryLowering<tensorium::PtrCastOp>,
        MemoryLowering<tensorium::GetGlobalOp>,
        MemoryLowering<tensorium::AllocaOp>, MemoryLowering<tensorium::LoadOp>,
        MemoryLowering<tensorium::StoreOp>, MemoryLowering<tensorium::NullOp>,
        MemoryLowering<tensorium::PtrOffsetOp>,
        MemoryLowering<tensorium::ArrayDecayOp>,
        MemoryLowering<tensorium::PtrCmpOp>,
        MemoryLowering<tensorium::PtrDiffOp>>(converter, &getContext());
    arith::populateArithToLLVMConversionPatterns(converter, patterns);
    cf::populateControlFlowToLLVMConversionPatterns(converter, patterns);
    populateFuncToLLVMConversionPatterns(converter, patterns);
    LLVMConversionTarget target(getContext());
    target.addLegalOp<ModuleOp>();
    target.addIllegalDialect<tensorium::TensoriumDialect>();
    target.addIllegalOp<UnrealizedConversionCastOp>();
    if (failed(
            applyFullConversion(getOperation(), target, std::move(patterns)))) {
      signalPassFailure();
      return;
    }
    // Type-valued metadata is not rewritten by the standard conversion
    // patterns. Do not return a mixed representation hidden in attributes.
    auto residual = getOperation().walk([&](Operation *op) -> WalkResult {
      bool found = false;
      auto check = [&](Attribute attributes) {
        if (attributes)
          attributes.walk([&](Type type) {
            found |= type.getDialect().getNamespace() == "tensorium";
          });
      };
      check(op->getAttrDictionary());
      check(op->getPropertiesAsAttribute());
      if (found) {
        op->emitOpError(
            "Tensorium type remains in unsupported type-valued metadata");
        return WalkResult::interrupt();
      }
      return WalkResult::advance();
    });
    if (residual.wasInterrupted())
      signalPassFailure();
  }
};
} // namespace

std::unique_ptr<Pass> tensorium::createTensoriumToLLVMPass() {
  return std::make_unique<TensoriumToLLVMPass>();
}
void tensorium::registerTensoriumToLLVMPass() {
  static PassRegistration<TensoriumToLLVMPass> registration;
}
