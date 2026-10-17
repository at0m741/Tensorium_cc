#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlowOps.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Parser/Parser.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassManager.h"
#include "tensorium/Conversion/TensoriumToMemRef/TensoriumToMemRef.h"
#include "tensorium/Dialect/Tensorium/IR/TensoriumDialect.h"
#include "llvm/Support/raw_ostream.h"
#include <iostream>
#include <stdexcept>

#define EXPECT(condition)                                                      \
  do {                                                                         \
    if (!(condition))                                                          \
      throw std::runtime_error(#condition);                                    \
  } while (false)

namespace {

mlir::LogicalResult lower(mlir::ModuleOp module) {
  mlir::PassManager manager(module.getContext());
  manager.addPass(tensorium::createTensoriumToMemRefPass());
  return manager.run(module);
}

void lowersScalarStorage(mlir::MLIRContext &context) {
  for (const char *type : {"i1", "i16", "i32", "i64", "f32", "f64"}) {
    std::string source = std::string("module { func.func @roundtrip(%v: ") +
                         type + ") -> " + type +
                         " { %p = tensorium.alloca : !tensorium.ptr<" + type +
                         "> tensorium.store %v, %p : !tensorium.ptr<" + type +
                         "> %r = tensorium.load %p : !tensorium.ptr<" + type +
                         "> return %r : " + type + " } }";
    auto module = mlir::parseSourceString<mlir::ModuleOp>(source, &context);
    EXPECT(module && mlir::succeeded(lower(*module)));
    EXPECT(mlir::succeeded(mlir::verify(*module)));
    unsigned allocas = 0, stores = 0, loads = 0;
    module->walk([&](mlir::memref::AllocaOp op) {
      EXPECT(op.getType().getRank() == 0);
      ++allocas;
    });
    module->walk([&](mlir::memref::StoreOp) { ++stores; });
    module->walk([&](mlir::memref::LoadOp) { ++loads; });
    EXPECT(allocas == 1 && stores == 1 && loads == 1);
    std::string text;
    llvm::raw_string_ostream stream(text);
    module->print(stream);
    EXPECT(text.find("tensorium.") == std::string::npos);
    EXPECT(text.find("unrealized_conversion_cast") == std::string::npos);
    EXPECT(mlir::parseSourceString<mlir::ModuleOp>(text, &context));
    // Applying the compatibility conversion twice must preserve legal IR.
    EXPECT(mlir::succeeded(lower(*module)));
  }
}

void convertsCallsReturnsAndCFG(mlir::MLIRContext &context) {
  auto module = mlir::parseSourceString<mlir::ModuleOp>(R"mlir(
    module {
      func.func private @identity(%p: !tensorium.ptr<i32>)
          -> !tensorium.ptr<i32> {
        return %p : !tensorium.ptr<i32>
      }
      func.func @choose(%value: i32, %condition: i1) -> i32 {
        %p = tensorium.alloca : !tensorium.ptr<i32>
        tensorium.store %value, %p : !tensorium.ptr<i32>
        %q = func.call @identity(%p)
          : (!tensorium.ptr<i32>) -> !tensorium.ptr<i32>
        cf.cond_br %condition, ^left(%q : !tensorium.ptr<i32>),
                               ^right(%p : !tensorium.ptr<i32>)
      ^left(%a: !tensorium.ptr<i32>):
        cf.br ^exit(%a : !tensorium.ptr<i32>)
      ^right(%b: !tensorium.ptr<i32>):
        cf.br ^exit(%b : !tensorium.ptr<i32>)
      ^exit(%result: !tensorium.ptr<i32>):
        %v = tensorium.load %result : !tensorium.ptr<i32>
        return %v : i32
      }
    }
  )mlir",
                                                        &context);
  EXPECT(module && mlir::succeeded(lower(*module)));
  EXPECT(mlir::succeeded(mlir::verify(*module)));
  auto identity = module->lookupSymbol<mlir::func::FuncOp>("identity");
  EXPECT(mlir::isa<mlir::MemRefType>(identity.getArgument(0).getType()));
  EXPECT(mlir::isa<mlir::MemRefType>(identity.getFunctionType().getResult(0)));
  auto choose = module->lookupSymbol<mlir::func::FuncOp>("choose");
  for (auto &block : llvm::drop_begin(choose.getBody())) {
    EXPECT(block.getNumArguments() == 1);
    EXPECT(mlir::isa<mlir::MemRefType>(block.getArgument(0).getType()));
  }
  choose.walk([](mlir::func::CallOp call) {
    EXPECT(mlir::isa<mlir::MemRefType>(call.getOperand(0).getType()));
    EXPECT(mlir::isa<mlir::MemRefType>(call.getResult(0).getType()));
  });
}

void rejectsUnsupportedRepresentations(mlir::MLIRContext &context) {
  struct Case {
    const char *source;
    const char *diagnostic;
  };
  const Case cases[] = {
      {R"(module { func.func @bad() {
          %p = tensorium.alloca : !tensorium.ptr<!tensorium.ptr<i32>>
          return } })",
       "only supports scalar pointee types"},
      {R"(module { func.func private @external(!tensorium.ptr<i32>) })",
       "explicit boundary conversion"},
      {R"(module { func.func private @external() -> !tensorium.ptr<i32> })",
       "explicit boundary conversion"},
      {R"(module { func.func @bad(%p: !tensorium.ptr<i32>) {
          "test.escape"(%p) : (!tensorium.ptr<i32>) -> ()
          return } })",
       "failed to legalize"},
  };
  context.allowUnregisteredDialects();
  for (const auto &test : cases) {
    auto module =
        mlir::parseSourceString<mlir::ModuleOp>(test.source, &context);
    EXPECT(module);
    std::string diagnostics;
    mlir::ScopedDiagnosticHandler handler(
        &context, [&](mlir::Diagnostic &diag) {
          llvm::raw_string_ostream stream(diagnostics);
          diag.print(stream);
          return mlir::success();
        });
    EXPECT(mlir::failed(lower(*module)));
    EXPECT(diagnostics.find(test.diagnostic) != std::string::npos);
  }
}

} // namespace

int main() {
  try {
    mlir::MLIRContext context;
    context.loadDialect<tensorium::TensoriumDialect, mlir::arith::ArithDialect,
                        mlir::func::FuncDialect, mlir::cf::ControlFlowDialect,
                        mlir::memref::MemRefDialect>();
    lowersScalarStorage(context);
    convertsCallsReturnsAndCFG(context);
    rejectsUnsupportedRepresentations(context);
    std::cout << "Tensorium-to-MemRef tests passed\n";
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
