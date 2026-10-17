#include "mlir/AsmParser/AsmParser.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/Parser/Parser.h"
#include "tensorium/Dialect/Tensorium/IR/TensoriumDialect.h"
#include "tensorium/Dialect/Tensorium/IR/TensoriumOps.h"
#include "llvm/Support/raw_ostream.h"
#include <iostream>
#include <stdexcept>

#define EXPECT(condition)                                                      \
  do {                                                                         \
    if (!(condition))                                                          \
      throw std::runtime_error(#condition);                                    \
  } while (false)

namespace {

void typesRoundTrip(mlir::MLIRContext &context) {
  for (const char *text : {"!tensorium.ptr<i32>", "!tensorium.ptr<f64>",
                           "!tensorium.ptr<!tensorium.ptr<i32>>"}) {
    auto type = mlir::parseType(text, &context);
    EXPECT(type && mlir::isa<tensorium::PtrType>(type));
    std::string output;
    llvm::raw_string_ostream stream(output);
    type.print(stream);
    EXPECT(output == text);
    EXPECT(mlir::parseType(output, &context) == type);
  }
  for (const char *text :
       {"!tensorium.ptr<none>", "!tensorium.ptr<vector<4xi32>>",
        "!tensorium.ptr<() -> ()>", "!tensorium.ptr<>"}) {
    unsigned errors = 0;
    mlir::ScopedDiagnosticHandler handler(&context, [&](mlir::Diagnostic &) {
      ++errors;
      return mlir::success();
    });
    EXPECT(!mlir::parseType(text, &context));
    EXPECT(errors != 0);
  }
}

void checksMemoryEffects(mlir::MLIRContext &context) {
  auto module = mlir::parseSourceString<mlir::ModuleOp>(R"mlir(
    module {
      func.func @roundtrip(%value: i32) -> i32 {
        %p = tensorium.alloca : !tensorium.ptr<i32>
        tensorium.store %value, %p : !tensorium.ptr<i32>
        %result = tensorium.load %p : !tensorium.ptr<i32>
        return %result : i32
      }
    }
  )mlir",
                                                        &context);
  EXPECT(module);
  unsigned checked = 0;
  module->walk([&](mlir::Operation *op) {
    if (op->getName().getDialectNamespace() != "tensorium")
      return;
    auto interface = mlir::dyn_cast<mlir::MemoryEffectOpInterface>(op);
    EXPECT(interface);
    llvm::SmallVector<mlir::MemoryEffects::EffectInstance> effects;
    interface.getEffects(effects);
    EXPECT(effects.size() == 1);
    if (auto allocation = mlir::dyn_cast<tensorium::AllocaOp>(op)) {
      EXPECT(mlir::isa<mlir::MemoryEffects::Allocate>(effects[0].getEffect()));
      EXPECT(effects[0].getValue() == allocation.getResult());
      EXPECT(effects[0].getResource() ==
             mlir::SideEffects::AutomaticAllocationScopeResource::get());
    } else if (auto load = mlir::dyn_cast<tensorium::LoadOp>(op)) {
      EXPECT(mlir::isa<mlir::MemoryEffects::Read>(effects[0].getEffect()));
      EXPECT(effects[0].getValue() == load.getAddress());
    } else {
      auto store = mlir::cast<tensorium::StoreOp>(op);
      EXPECT(mlir::isa<mlir::MemoryEffects::Write>(effects[0].getEffect()));
      EXPECT(effects[0].getValue() == store.getAddress());
    }
    ++checked;
  });
  EXPECT(checked == 3);
  std::string text;
  llvm::raw_string_ostream stream(text);
  module->print(stream);
  EXPECT(mlir::parseSourceString<mlir::ModuleOp>(text, &context));
}

void rejectsInvalidMemoryIR(mlir::MLIRContext &context) {
  const char *cases[] = {
      "module { %p = tensorium.alloca : !tensorium.ptr<i32> }",
      "module { func.func @bad() { %p = tensorium.alloca : i32 return } }",
      R"(module { func.func @bad(%p: !tensorium.ptr<i32>) {
           %v = "tensorium.load"(%p) : (!tensorium.ptr<i32>) -> f64
           return } })",
      R"(module { func.func @bad(%v: i64, %p: !tensorium.ptr<i32>) {
           "tensorium.store"(%v, %p) : (i64, !tensorium.ptr<i32>) -> ()
           return } })",
      R"(module { func.func @bad(%v: i32, %p: !tensorium.ptr<i32>) {
           %r = "tensorium.store"(%v, %p) : (i32, !tensorium.ptr<i32>) -> i32
           return } })",
  };
  for (const char *source : cases) {
    unsigned errors = 0;
    mlir::ScopedDiagnosticHandler handler(&context, [&](mlir::Diagnostic &) {
      ++errors;
      return mlir::success();
    });
    EXPECT(!mlir::parseSourceString<mlir::ModuleOp>(source, &context));
    EXPECT(errors != 0);
  }
}

} // namespace

int main() {
  try {
    mlir::DialectRegistry registry;
    registry.insert<tensorium::TensoriumDialect, mlir::func::FuncDialect>();
    mlir::MLIRContext context(registry);
    EXPECT(!context.getLoadedDialect("tensorium"));
    auto *dialect = context.getOrLoadDialect<tensorium::TensoriumDialect>();
    EXPECT(dialect == context.getOrLoadDialect<tensorium::TensoriumDialect>());
    typesRoundTrip(context);
    checksMemoryEffects(context);
    rejectsInvalidMemoryIR(context);
    std::cout << "Tensorium IR tests passed\n";
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
