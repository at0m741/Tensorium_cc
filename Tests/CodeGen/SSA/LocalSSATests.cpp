#include "cc1/Basic/LangOptions.hpp"
#include "cc1/CodeGen/LocalStorageAnalysis.hpp"
#include "cc1/CodeGen/MLIRGen.hpp"
#include "cc1/Lex/Lexer.hpp"
#include "cc1/Parse/Parser.hpp"
#include "cc1/Sema/Sema.hpp"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/IR/Verifier.h"
#include "tensorium/Dialect/Tensorium/IR/TensoriumOps.h"
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
void expect(bool condition, const std::string &message) {
  if (!condition)
    throw std::runtime_error(message);
}

std::string readSource(const std::string &path) {
  std::ifstream input(path);
  if (!input)
    throw std::runtime_error("cannot open fixture: " + path);
  return std::string(std::istreambuf_iterator<char>(input),
                     std::istreambuf_iterator<char>());
}

struct Fixture {
  std::string path;
  std::string source;
  std::ostringstream output;
  DiagnosticEngine diag;
  LangOptions options = LangOptions::forc99();
  TargetInfo target = TargetInfo::x86_64();
  ASTContext ast;
  Lexer lexer;
  Parser parser;
  TranslationUnit *unit;
  Sema sema;
  mlir::MLIRContext context;
  MLIRGen generator;

  explicit Fixture(const char *filename)
      : path(std::string(CC1_SSA_TEST_SOURCE_DIR) + "/" + filename),
        source(readSource(path)),
        diag(path, source, output, DiagnosticColor::Never),
        lexer(source, path.c_str(), diag, options),
        parser(lexer, ast, diag, target, options), unit(parser.parse()),
        sema(ast, diag, target), generator(context, diag, target) {
    if (!unit || diag.hasErrors() || !sema.analyze(*unit))
      throw std::runtime_error(output.str());
  }

  const FuncDecl &function(const std::string &name) const {
    for (const auto *decl : unit->decls)
      if (auto *fn = dynamic_cast<const FuncDecl *>(decl))
        if (fn->name == name && fn->body)
          return *fn;
    throw std::runtime_error("missing function: " + name);
  }

  mlir::OwningOpRef<mlir::ModuleOp> generate() {
    auto module = generator.generate(*unit);
    if (!module || diag.hasErrors())
      throw std::runtime_error(output.str());
    expect(mlir::succeeded(mlir::verify(module.get())), "invalid raw MLIR");
    return module;
  }
};

mlir::func::FuncOp function(mlir::ModuleOp module, const char *name) {
  auto fn = module.lookupSymbol<mlir::func::FuncOp>(name);
  if (!fn)
    throw std::runtime_error(std::string("missing MLIR function: ") + name);
  return fn;
}

const VarDecl &local(const FuncDecl &fn, const std::string &name) {
  for (const auto *item : fn.body->items)
    if (auto *var = dynamic_cast<const VarDecl *>(item))
      if (var->name == name)
        return *var;
  throw std::runtime_error("missing local: " + fn.name + ":" + name);
}

void expectNoMemory(mlir::func::FuncOp fn) {
  fn.walk([&](mlir::Operation *op) {
    expect(!mlir::isa<tensorium::AllocaOp, tensorium::LoadOp,
                      tensorium::StoreOp>(op),
           fn.getSymName().str() + ": unexpected " +
               op->getName().getStringRef().str());
  });
}

void expectAllocations(mlir::func::FuncOp fn, unsigned expected) {
  unsigned count = 0;
  auto *entry = &fn.getBody().front();
  fn.walk([&](tensorium::AllocaOp allocation) {
    ++count;
    expect(allocation->getBlock() == entry,
           fn.getSymName().str() + ": unexpected allocation inside control flow");
  });
  expect(count == expected, fn.getSymName().str() + ": expected " +
                                std::to_string(expected) + " allocations, got " +
                                std::to_string(count));
}

void expectOneInitializerCall(mlir::func::FuncOp fn) {
  std::vector<mlir::func::CallOp> calls;
  fn.walk([&](mlir::func::CallOp call) { calls.push_back(call); });
  expect(calls.size() == 1,
         fn.getSymName().str() + ": initializer must be evaluated once");
  auto ret = mlir::cast<mlir::func::ReturnOp>(fn.getBody().front().back());
  auto add = ret.getOperands().front().getDefiningOp<mlir::arith::AddIOp>();
  expect(static_cast<bool>(add), "expected addition of the initialized local");
  expect(add.getLhs() == calls.front().getResult(0) &&
             add.getRhs() == calls.front().getResult(0),
         fn.getSymName().str() + ": local reads must reuse the call result");
}

void analyzesLocalStorage() {
  Fixture values("locals.c");
  Fixture memory("locals_memory.c");
  Fixture guards("locals_analysis_only.c");
  struct Case {
    Fixture *fixture;
    const char *function;
    const char *local;
    bool storage;
  };
  const Case cases[] = {
      {&values, "local_scalar", "y", false},
      {&values, "local_const", "y", false},
      {&values, "local_narrow", "y", false},
      {&values, "local_pointer", "q", false},
      {&values, "local_snapshot", "before", false},
      {&values, "local_call", "y", false},
      {&values, "local_initializer_write", "y", false},
      {&values, "local_sizeof_self", "y", false},
      {&values, "local_sizeof_effects", "y", false},
      {&memory, "local_reassigned", "y", true},
      {&memory, "local_uninitialized", "y", true},
      {&memory, "local_branch_write", "y", true},
      {&memory, "local_address", "y", true},
      {&memory, "local_address", "p", false},
      {&memory, "local_pointer_alias", "y", true},
      {&memory, "local_pointer_alias", "p", false},
      {&memory, "local_pointer_alias", "q", false},
      {&memory, "local_pointer_rebind", "p", true},
      {&memory, "local_copy_of_mutable", "y", false},
      {&memory, "local_array", "values", true},
      {&guards, "local_self_reference", "value", true},
      {&guards, "local_nested_self_reference", "value", true},
      {&guards, "local_pointer_self_reference", "value", true},
      {&guards, "local_volatile", "value", true},
      {&guards, "local_static", "value", true},
      {&guards, "local_extern", "value", true},
      {&values, "local_scalar", "y", false},
  };
  LocalStorageAnalysis analysis;
  for (const auto &test : cases) {
    const auto &fn = test.fixture->function(test.function);
    analysis.analyze(fn);
    expect(analysis.needsStorage(&local(fn, test.local)) == test.storage,
           fn.name + ": wrong storage decision for " + test.local);
  }
}

void keepsInitializedLocalsInSSA() {
  Fixture fixture("locals.c");
  auto module = fixture.generate();
  for (const char *name : {"local_scalar", "local_chain", "local_const",
                           "local_narrow", "local_call", "local_sizeof_self",
                           "local_sizeof_effects"})
    expectNoMemory(function(module.get(), name));

  auto scalar = function(module.get(), "local_scalar");
  auto ret = mlir::cast<mlir::func::ReturnOp>(scalar.getBody().front().back());
  auto mul = ret.getOperands().front().getDefiningOp<mlir::arith::MulIOp>();
  expect(static_cast<bool>(mul), "local_scalar: expected multiplication");
  auto add = mul.getLhs().getDefiningOp<mlir::arith::AddIOp>();
  expect(add && add.getLhs() == scalar.getArgument(0),
         "local_scalar: expected the initializer value directly in SSA");

  auto narrow = function(module.get(), "local_narrow");
  ret = mlir::cast<mlir::func::ReturnOp>(narrow.getBody().front().back());
  auto extension =
      ret.getOperands().front().getDefiningOp<mlir::arith::ExtSIOp>();
  expect(extension && extension.getIn() == narrow.getArgument(0),
         "local_narrow: short-to-int conversion was not preserved");

  auto pointer = function(module.get(), "local_pointer");
  expectAllocations(pointer, 0);
  ret = mlir::cast<mlir::func::ReturnOp>(pointer.getBody().front().back());
  auto load = ret.getOperands().front().getDefiningOp<tensorium::LoadOp>();
  expect(load && load.getAddress() == pointer.getArgument(0),
         "local_pointer: expected a direct pointee load");

  auto snapshot = function(module.get(), "local_snapshot");
  expectAllocations(snapshot, 0);
  ret = mlir::cast<mlir::func::ReturnOp>(snapshot.getBody().front().back());
  add = ret.getOperands().front().getDefiningOp<mlir::arith::AddIOp>();
  expect(static_cast<bool>(add), "local_snapshot: expected addition");
  auto before = add.getLhs().getDefiningOp<tensorium::LoadOp>();
  auto after = add.getRhs().getDefiningOp<tensorium::LoadOp>();
  expect(before && after && before.getResult() != after.getResult(),
         "local_snapshot: distinct pointee reads must be preserved");
  expect(before.getAddress() == snapshot.getArgument(0) &&
             after.getAddress() == snapshot.getArgument(0),
         "local_snapshot: unexpected pointee addresses");

  expectOneInitializerCall(function(module.get(), "local_call"));
  expectAllocations(function(module.get(), "local_initializer_write"), 1);
}

void keepsLoopTemporariesOutOfMemory() {
  Fixture fixture("locals_control_flow.c");
  auto module = fixture.generate();
  expectNoMemory(function(module.get(), "local_branch"));
  expectNoMemory(function(module.get(), "local_shadow"));
  // Only mutable counters and accumulators need slots. In particular, there
  // must be no allocation for a temporary executed on every loop iteration.
  expectAllocations(function(module.get(), "local_for"), 2);
  expectAllocations(function(module.get(), "local_while"), 2);
  expectAllocations(function(module.get(), "local_for_seed"), 1);
  expectAllocations(function(module.get(), "local_loop_jumps"), 2);
}

void preservesAddressableAndMutableLocals() {
  Fixture fixture("locals_memory.c");
  auto module = fixture.generate();
  for (const char *name : {"local_reassigned", "local_uninitialized",
                           "local_branch_write", "local_address",
                           "local_pointer_alias", "local_pointer_rebind",
                           "local_initializer_address", "local_copy_of_mutable",
                           "local_array"})
    expectAllocations(function(module.get(), name), 1);
  expectAllocations(function(module.get(), "local_address_in_loop"), 2);
  expectOneInitializerCall(function(module.get(), "local_initializer_address"));

  auto rebind = function(module.get(), "local_pointer_rebind");
  rebind.walk([](tensorium::AllocaOp allocation) {
    auto pointer =
        mlir::cast<tensorium::PtrType>(allocation.getResult().getType());
    expect(mlir::isa<tensorium::PtrType>(pointer.getPointeeType()),
           "local_pointer_rebind: reassigned pointer needs pointer storage");
  });
}

void hoistsLoopObjectStorage() {
  Fixture fixture("loop_storage.c");
  auto module = fixture.generate();
  for (const char *name : {"loop_for_storage", "loop_while_storage",
                           "loop_address_storage", "loop_array_storage",
                           "loop_call_initializer", "loop_conditional_initializer",
                           "loop_load_initializer"})
    expectAllocations(function(module.get(), name), 3);
  // The two shadowed values and the two counters have independent slots.
  expectAllocations(function(module.get(), "loop_nested_storage"), 5);

  auto array = function(module.get(), "loop_array_storage");
  unsigned arraySlots = 0;
  array.walk([&](tensorium::AllocaOp allocation) {
    auto pointer =
        mlir::cast<tensorium::PtrType>(allocation.getResult().getType());
    if (mlir::isa<tensorium::ArrayType>(pointer.getPointeeType()))
      ++arraySlots;
  });
  expect(arraySlots == 1, "loop_array_storage: expected one fixed-size array slot");
}

void preservesLoopInitializerPlacement() {
  Fixture fixture("loop_storage.c");
  auto module = fixture.generate();
  for (const char *name : {"loop_call_initializer",
                           "loop_conditional_initializer"}) {
    auto fn = function(module.get(), name);
    auto *entry = &fn.getBody().front();
    unsigned calls = 0;
    fn.walk([&](mlir::func::CallOp call) {
      if (call.getCallee() != "loop_next")
        return;
      ++calls;
      expect(call->getBlock() != entry,
             fn.getSymName().str() + ": initializer call was hoisted");
      unsigned initializations = 0;
      fn.walk([&](tensorium::StoreOp store) {
        if (store->getOperand(0) != call.getResult(0))
          return;
        auto slot = store.getAddress().getDefiningOp<tensorium::AllocaOp>();
        expect(slot && slot->getBlock() == entry,
               fn.getSymName().str() + ": initializer needs entry storage");
        expect(store->getBlock() == call->getBlock(),
               fn.getSymName().str() + ": initialization moved away from its call");
        ++initializations;
      });
      expect(initializations == 1,
             fn.getSymName().str() + ": expected one store of the initializer");
    });
    expect(calls == 1, fn.getSymName().str() + ": expected one initializer call");
  }

  auto loadFn = function(module.get(), "loop_load_initializer");
  unsigned initializerLoads = 0;
  loadFn.walk([&](tensorium::LoadOp load) {
    if (load.getAddress() != loadFn.getArgument(1))
      return;
    ++initializerLoads;
    expect(load->getBlock() != &loadFn.getBody().front(),
           "loop_load_initializer: initializer load was hoisted");
  });
  expect(initializerLoads == 1,
         "loop_load_initializer: expected one pointee load in the loop");
}
} // namespace

int main() {
  struct Test {
    const char *name;
    void (*run)();
  };
  const Test tests[] = {
      {"analyzes_local_storage", analyzesLocalStorage},
      {"keeps_initialized_locals_in_ssa", keepsInitializedLocalsInSSA},
      {"keeps_loop_temporaries_out_of_memory", keepsLoopTemporariesOutOfMemory},
      {"preserves_addressable_and_mutable_locals",
       preservesAddressableAndMutableLocals},
      {"hoists_loop_object_storage", hoistsLoopObjectStorage},
      {"preserves_loop_initializer_placement", preservesLoopInitializerPlacement},
  };
  unsigned failures = 0;
  for (const auto &test : tests) {
    try {
      test.run();
      std::cout << "[PASSED] " << test.name << '\n';
    } catch (const std::exception &error) {
      ++failures;
      std::cerr << "[FAILED] " << test.name << ": " << error.what() << '\n';
    }
  }
  return failures ? 1 : 0;
}
