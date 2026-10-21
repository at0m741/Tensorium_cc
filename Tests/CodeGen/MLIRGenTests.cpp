#include "cc1/Basic/LangOptions.hpp"
#include "cc1/CodeGen/MLIRGen.hpp"
#include "cc1/Lex/Lexer.hpp"
#include "cc1/Parse/Parser.hpp"
#include "cc1/Sema/Sema.hpp"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlowOps.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "tensorium/Dialect/Tensorium/IR/TensoriumOps.h"
#include "mlir/IR/Verifier.h"
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>

#define EXPECT(condition)                                                      \
  do {                                                                         \
    if (!(condition))                                                          \
      throw std::runtime_error(#condition);                                    \
  } while (false)

struct Context {
  std::string source;
  std::ostringstream output;
  DiagnosticEngine diag;
  LangOptions options = LangOptions::forc99();
  TargetInfo target;
  ASTContext ast;
  TypePool &types = ast.getTypes();
  Lexer lexer;
  Parser parser;
  TranslationUnit *unit;
  Sema sema;
  mlir::MLIRContext mlirContext;
  MLIRGen generator;

  explicit Context(std::string text, TargetInfo info = TargetInfo::x86_64())
      : source(std::move(text)),
        diag("mlir.c", source, output, DiagnosticColor::Never), target(info),
        lexer(source, "mlir.c", diag, options),
        parser(lexer, ast, diag, target, options), unit(parser.parse()),
        sema(ast, diag, target), generator(mlirContext, diag, target) {
    if (diag.hasErrors() || !sema.analyze(*unit))
      throw std::runtime_error(output.str());
  }

  mlir::OwningOpRef<mlir::ModuleOp> generate() {
    auto module = generator.generate(*unit);
    if (!module)
      throw std::runtime_error(output.str());
    EXPECT(mlir::succeeded(mlir::verify(module.get())));
    EXPECT(!diag.hasErrors());
    module->walk([](mlir::Operation *op) {
      EXPECT(op->getName().getDialectNamespace() != "memref");
    });
    return module;
  }
};

void maps_parameters_from_declarations() {
  Context c("int add(int a, int b) { return a + b; }");
  auto module = c.generate();
  auto fn = module->lookupSymbol<mlir::func::FuncOp>("add");
  EXPECT(fn);
  auto &block = fn.getBody().front();
  EXPECT(block.getNumArguments() == 2);
  EXPECT(block.getOperations().size() == 2);
  auto add = mlir::dyn_cast<mlir::arith::AddIOp>(block.front());
  EXPECT(add && add.getLhs() == block.getArgument(0));
  EXPECT(add.getRhs() == block.getArgument(1));
  auto ret = mlir::dyn_cast<mlir::func::ReturnOp>(block.back());
  EXPECT(ret && ret.getOperands().front() == add.getResult());
}

void lowers_nested_binary_expressions() {
  Context c("int expression(int a, int b) { return (a - b) * (a + b) + 3; }");
  auto module = c.generate();
  auto fn = module->lookupSymbol<mlir::func::FuncOp>("expression");
  std::vector<mlir::Operation *> ops;
  for (auto &op : fn.getBody().front())
    ops.push_back(&op);
  EXPECT(ops.size() == 6);
  auto sub = mlir::dyn_cast<mlir::arith::SubIOp>(ops[0]);
  auto innerAdd = mlir::dyn_cast<mlir::arith::AddIOp>(ops[1]);
  auto mul = mlir::dyn_cast<mlir::arith::MulIOp>(ops[2]);
  auto constant = mlir::dyn_cast<mlir::arith::ConstantIntOp>(ops[3]);
  auto outerAdd = mlir::dyn_cast<mlir::arith::AddIOp>(ops[4]);
  EXPECT(sub && innerAdd && mul && constant && outerAdd);
  EXPECT(constant.value() == 3);
  EXPECT(mul.getLhs() == sub.getResult() &&
         mul.getRhs() == innerAdd.getResult());
  EXPECT(outerAdd.getLhs() == mul.getResult() &&
         outerAdd.getRhs() == constant.getResult());
}

void keeps_parameter_bindings_local_to_each_function() {
  Context c("int first(int a, int b) { return a - b; } int second(int b, int "
            "a) { return a + b; }");
  auto module = c.generate();
  auto first = module->lookupSymbol<mlir::func::FuncOp>("first");
  auto second = module->lookupSymbol<mlir::func::FuncOp>("second");
  auto &block = second.getBody().front();
  auto add = mlir::dyn_cast<mlir::arith::AddIOp>(block.front());
  EXPECT(add && add.getLhs() == block.getArgument(1));
  EXPECT(add.getRhs() == block.getArgument(0));
  EXPECT(add.getLhs() != first.getBody().front().getArgument(1));
}

void lowers_integer_promotions() {
  Context c("int promote(short a, unsigned short b) { return a + b; }");
  auto module = c.generate();
  auto fn = module->lookupSymbol<mlir::func::FuncOp>("promote");
  auto &block = fn.getBody().front();
  std::vector<mlir::Operation *> ops;
  for (auto &op : block)
    ops.push_back(&op);
  EXPECT(ops.size() == 4);
  auto signedExtend = mlir::dyn_cast<mlir::arith::ExtSIOp>(ops[0]);
  auto unsignedExtend = mlir::dyn_cast<mlir::arith::ExtUIOp>(ops[1]);
  auto add = mlir::dyn_cast<mlir::arith::AddIOp>(ops[2]);
  EXPECT(signedExtend && unsignedExtend && add);
  EXPECT(signedExtend.getIn() == block.getArgument(0));
  EXPECT(unsignedExtend.getIn() == block.getArgument(1));
  EXPECT(add.getLhs() == signedExtend.getResult());
  EXPECT(add.getRhs() == unsignedExtend.getResult());
  EXPECT(add.getType().isInteger(32));
}

void lowers_integer_to_bool_conversions() {
  for (auto target : {TargetInfo::i386(), TargetInfo::x86_64()}) {
    for (auto kind : {Type::CHAR, Type::INT, Type::LONG}) {
      // The parser does not accept _Bool yet; exercise a typed AST directly.
      ASTContext ast;
      TypePool &types = ast.getTypes();
      ParamDecl parameter;
      parameter.type = types.make(kind);
      IdentExpr reference;
      reference.type = parameter.type;
      reference.decl = &parameter;
      ImplicitCastExpr conversion;
      conversion.kind = ImplicitCastKind::Arithmetic;
      conversion.type = types.make(Type::BOOL);
      conversion.operand = &reference;
      ImplicitCastExpr promotion;
      promotion.kind = ImplicitCastKind::Arithmetic;
      promotion.type = types.intTy();
      promotion.operand = &conversion;
      ReturnStmt returnStmt;
      returnStmt.value = &promotion;
      CompoundStmt body;
      body.items.push_back(&returnStmt);
      FuncDecl function;
      function.name = "to_bool";
      function.type = types.function(promotion.type, {parameter.type});
      function.params.push_back(&parameter);
      function.body = &body;
      TranslationUnit unit;
      unit.decls.push_back(&function);

      std::ostringstream output;
      DiagnosticEngine diag("mlir.c", "", output, DiagnosticColor::Never);
      mlir::MLIRContext context;
      MLIRGen generator(context, diag, target);
      auto module = generator.generate(unit);
      if (!module)
        throw std::runtime_error(output.str());
      EXPECT(!diag.hasErrors());
      EXPECT(mlir::succeeded(mlir::verify(module.get())));
      auto fn = module->lookupSymbol<mlir::func::FuncOp>("to_bool");
      auto &block = fn.getBody().front();
      auto ret = mlir::cast<mlir::func::ReturnOp>(block.back());
      auto extension =
          ret.getOperands().front().getDefiningOp<mlir::arith::ExtUIOp>();
      EXPECT(extension && extension.getType().isInteger(32));
      auto comparison = extension.getIn().getDefiningOp<mlir::arith::CmpIOp>();
      EXPECT(comparison);
      EXPECT(comparison.getPredicate() == mlir::arith::CmpIPredicate::ne);
      EXPECT(comparison.getLhs() == block.getArgument(0));
      auto zero =
          comparison.getRhs().getDefiningOp<mlir::arith::ConstantIntOp>();
      EXPECT(zero && zero.value() == 0);
      EXPECT(zero.getType() == block.getArgument(0).getType());
      EXPECT(comparison.getType().isInteger(1));
    }
  }
}

void lowers_target_dependent_integer_conversions() {
  for (auto target : {TargetInfo::i386(), TargetInfo::x86_64()}) {
    Context c("int widen(int a, long b) { return a + b; } int "
              "unsigned_widen(unsigned int a, long b) { return a + b; }",
              target);
    auto module = c.generate();
    for (const char *name : {"widen", "unsigned_widen"}) {
      auto fn = module->lookupSymbol<mlir::func::FuncOp>(name);
      auto &block = fn.getBody().front();
      EXPECT(block.getArgument(1).getType().isInteger(target.sizeofLong * 8));
      if (target.sizeofLong == 8) {
        EXPECT(block.getOperations().size() == 4);
        EXPECT(std::string(name) == "widen"
                   ? mlir::isa<mlir::arith::ExtSIOp>(block.front())
                   : mlir::isa<mlir::arith::ExtUIOp>(block.front()));
        auto ret = mlir::cast<mlir::func::ReturnOp>(block.back());
        auto truncate =
            ret.getOperands().front().getDefiningOp<mlir::arith::TruncIOp>();
        EXPECT(truncate && truncate.getType().isInteger(32));
      } else {
        EXPECT(block.getOperations().size() == 2);
        EXPECT(mlir::isa<mlir::arith::AddIOp>(block.front()));
      }
    }
  }
}

void lowers_identity_constants_and_private_functions() {
  Context c("int identity(int x) { return x; } int constant(void) { return 1U "
            "+ 2; } static int local(void) { return 0; }");
  auto module = c.generate();
  auto identity = module->lookupSymbol<mlir::func::FuncOp>("identity");
  auto &block = identity.getBody().front();
  EXPECT(block.getOperations().size() == 1);
  EXPECT(mlir::cast<mlir::func::ReturnOp>(block.back()).getOperands().front() ==
         block.getArgument(0));
  auto constant = module->lookupSymbol<mlir::func::FuncOp>("constant");
  EXPECT(constant.getBody().front().getOperations().size() == 4);
  EXPECT(module->lookupSymbol<mlir::func::FuncOp>("local").isPrivate());
}

void lowers_locals_and_discarded_calls() {
  Context c("int add(int a, int b) { return a + b; } int main(void) { int a = "
            "1; int b = 4; add(a, b); }");
  auto module = c.generate();
  auto fn = module->lookupSymbol<mlir::func::FuncOp>("main");
  std::vector<tensorium::AllocaOp> slots;
  std::vector<tensorium::StoreOp> stores;
  std::vector<tensorium::LoadOp> loads;
  mlir::func::CallOp call;
  for (auto &op : fn.getBody().front()) {
    if (auto x = mlir::dyn_cast<tensorium::AllocaOp>(op))
      slots.push_back(x);
    if (auto x = mlir::dyn_cast<tensorium::StoreOp>(op))
      stores.push_back(x);
    if (auto x = mlir::dyn_cast<tensorium::LoadOp>(op))
      loads.push_back(x);
    if (auto x = mlir::dyn_cast<mlir::func::CallOp>(op))
      call = x;
  }
  EXPECT(slots.size() == 2 && stores.size() == 2 && loads.size() == 2);
  EXPECT(stores[0]->getOperand(1) == slots[0].getResult());
  EXPECT(stores[1]->getOperand(1) == slots[1].getResult());
  EXPECT(loads[0]->getOperand(0) == slots[0].getResult());
  EXPECT(loads[1]->getOperand(0) == slots[1].getResult());
  EXPECT(call && call.getCallee() == "add");
  EXPECT(call.getOperands()[0] == loads[0].getResult());
  EXPECT(call.getOperands()[1] == loads[1].getResult());
  EXPECT(call.getResult(0).use_empty());
  auto ret = mlir::cast<mlir::func::ReturnOp>(fn.getBody().front().back());
  auto zero =
      ret.getOperands().front().getDefiningOp<mlir::arith::ConstantIntOp>();
  EXPECT(zero && zero.value() == 0);
}

void respects_local_shadowing_and_mutation() {
  Context c("int f(void) { int x = 1; { int x = 2; } x = 3; return x; } int "
            "param(int x) { x = 4; return x; }");
  auto module = c.generate();
  auto fn = module->lookupSymbol<mlir::func::FuncOp>("f");
  std::vector<tensorium::AllocaOp> slots;
  std::vector<tensorium::StoreOp> stores;
  for (auto &op : fn.getBody().front()) {
    if (auto x = mlir::dyn_cast<tensorium::AllocaOp>(op))
      slots.push_back(x);
    if (auto x = mlir::dyn_cast<tensorium::StoreOp>(op))
      stores.push_back(x);
  }
  EXPECT(slots.size() == 2 && stores.size() == 3);
  EXPECT(stores[0]->getOperand(1) == slots[0].getResult());
  EXPECT(stores[1]->getOperand(1) == slots[1].getResult());
  EXPECT(stores[2]->getOperand(1) == slots[0].getResult());
  auto ret = mlir::cast<mlir::func::ReturnOp>(fn.getBody().front().back());
  auto load = ret.getOperands().front().getDefiningOp<tensorium::LoadOp>();
  EXPECT(load && load->getOperand(0) == slots[0].getResult());
  auto param = module->lookupSymbol<mlir::func::FuncOp>("param");
  auto paramRet =
      mlir::cast<mlir::func::ReturnOp>(param.getBody().front().back());
  EXPECT(paramRet.getOperands().front().getDefiningOp<tensorium::LoadOp>());
}

void shares_function_symbols_across_prototypes_and_definitions() {
  Context c("int add(int, int); int caller(void) { return add(1, 4); } int "
            "add(int a, int b) { return a + b; } int add(int, int);");
  auto module = c.generate();
  EXPECT(module->getBody()->getOperations().size() == 2);
  auto add = module->lookupSymbol<mlir::func::FuncOp>("add");
  EXPECT(add && !add.isExternal() && add.isPublic());
  auto caller = module->lookupSymbol<mlir::func::FuncOp>("caller");
  auto ret = mlir::cast<mlir::func::ReturnOp>(caller.getBody().front().back());
  auto call = ret.getOperands().front().getDefiningOp<mlir::func::CallOp>();
  EXPECT(call && call.getCallee() == "add" && call.getNumOperands() == 2);
}

void supports_external_void_nested_and_recursive_calls() {
  Context c("int external(int); void sink(int x) {} void caller(void) { "
            "sink(external(2)); } int recur(int x) { return recur(x); }");
  auto module = c.generate();
  auto external = module->lookupSymbol<mlir::func::FuncOp>("external");
  EXPECT(external && external.isExternal() && external.isPrivate());
  auto caller = module->lookupSymbol<mlir::func::FuncOp>("caller");
  std::vector<mlir::func::CallOp> calls;
  for (auto &op : caller.getBody().front())
    if (auto x = mlir::dyn_cast<mlir::func::CallOp>(op))
      calls.push_back(x);
  EXPECT(calls.size() == 2);
  EXPECT(calls[0].getCallee() == "external" && calls[1].getCallee() == "sink");
  EXPECT(calls[1].getOperands().front() == calls[0].getResult(0));
  EXPECT(calls[1].getNumResults() == 0);
  EXPECT(mlir::cast<mlir::func::ReturnOp>(caller.getBody().front().back())
             .getNumOperands() == 0);
  auto recur = module->lookupSymbol<mlir::func::FuncOp>("recur");
  auto ret = mlir::cast<mlir::func::ReturnOp>(recur.getBody().front().back());
  EXPECT(ret.getOperands()
             .front()
             .getDefiningOp<mlir::func::CallOp>()
             .getCallee() == "recur");
}

void converts_local_initializers_and_preserves_explicit_returns() {
  Context c("int f(void) { short x = 3; return x; } int main(void) { return 7; "
            "int ignored = 9; }");
  auto module = c.generate();
  auto fn = module->lookupSymbol<mlir::func::FuncOp>("f");
  tensorium::AllocaOp slot;
  for (auto &op : fn.getBody().front())
    if (auto x = mlir::dyn_cast<tensorium::AllocaOp>(op))
      slot = x;
  EXPECT(slot);
  auto pointerType = mlir::cast<tensorium::PtrType>(slot.getResult().getType());
  EXPECT(pointerType.getPointeeType().isInteger(16));
  auto ret = mlir::cast<mlir::func::ReturnOp>(fn.getBody().front().back());
  EXPECT(ret.getOperands().front().getDefiningOp<mlir::arith::ExtSIOp>());
  auto mainFn = module->lookupSymbol<mlir::func::FuncOp>("main");
  EXPECT(mainFn.getBody().front().getOperations().size() == 2);
  EXPECT(mlir::cast<mlir::func::ReturnOp>(mainFn.getBody().front().back())
             .getOperands()
             .front()
             .getDefiningOp<mlir::arith::ConstantIntOp>()
             .value() == 7);
}

void shares_parameter_storage_across_nested_control_flow() {
  Context c("int f(int n) { int result = 0; while (n > 0) { "
            "n = n - 1; if (n == 3) continue; if (n == 1) break; "
            "result = result + 1; } return result; }");
  auto module = c.generate();
  auto function = module->lookupSymbol<mlir::func::FuncOp>("f");
  auto &entry = function.getBody().front();
  unsigned allocations = 0;
  function.walk([&](tensorium::AllocaOp allocation) {
    ++allocations;
    EXPECT(allocation->getBlock() == &entry);
  });
  EXPECT(allocations == 2);
  auto branch = mlir::cast<mlir::cf::BranchOp>(entry.back());
  auto condition = mlir::cast<mlir::cf::CondBranchOp>(branch.getDest()->back());
  auto *exit = condition.getFalseDest();
  EXPECT(mlir::isa<mlir::func::ReturnOp>(exit->back()));
  unsigned continues = 0, breaks = 0;
  function.walk([&](mlir::cf::BranchOp jump) {
    if (jump->getBlock() != &entry && jump.getDest() == branch.getDest())
      ++continues;
    if (jump.getDest() == exit)
      ++breaks;
  });
  EXPECT(continues == 2 && breaks == 1);
}

mlir::cf::CondBranchOp comparisonBranch(mlir::arith::CmpIOp comparison) {
  return mlir::cast<mlir::cf::CondBranchOp>(comparison->getBlock()->back());
}

mlir::Block *forIncrement(mlir::func::FuncOp function, mlir::Value counter) {
  mlir::Block *increment = nullptr;
  function.walk([&](tensorium::StoreOp store) {
    if (store.getAddress() == counter &&
        store->getOperand(0).getDefiningOp<mlir::arith::AddIOp>()) {
      EXPECT(!increment);
      increment = store->getBlock();
    }
  });
  EXPECT(increment);
  return increment;
}

void routes_for_continue_through_increment_and_break_to_exit() {
  Context c("int count(int n) { int result = 0; "
            "for (int i = 0; i < n; i = i + 1) { "
            "if (i == 2) continue; if (i == 5) break; "
            "result = result + 1; } return result; }");
  auto module = c.generate();
  auto function = module->lookupSymbol<mlir::func::FuncOp>("count");
  mlir::arith::CmpIOp loop, skip, stop;
  function.walk([&](mlir::arith::CmpIOp comparison) {
    if (comparison.getPredicate() == mlir::arith::CmpIPredicate::slt)
      loop = comparison;
    if (comparison.getPredicate() != mlir::arith::CmpIPredicate::eq)
      return;
    auto constant =
        comparison.getRhs().getDefiningOp<mlir::arith::ConstantIntOp>();
    EXPECT(constant);
    if (constant.value() == 2)
      skip = comparison;
    if (constant.value() == 5)
      stop = comparison;
  });
  EXPECT(loop && skip && stop);
  auto counter = loop.getLhs().getDefiningOp<tensorium::LoadOp>();
  EXPECT(counter);
  auto *increment = forIncrement(function, counter.getAddress());
  auto *condition = loop->getBlock();
  auto *exit = comparisonBranch(loop).getFalseDest();
  EXPECT(mlir::cast<mlir::cf::BranchOp>(function.getBody().front().back())
             .getDest() == condition);
  EXPECT(mlir::cast<mlir::cf::BranchOp>(increment->back()).getDest() ==
         condition);
  EXPECT(mlir::cast<mlir::cf::BranchOp>(
             comparisonBranch(skip).getTrueDest()->back())
             .getDest() == increment);
  EXPECT(mlir::cast<mlir::cf::BranchOp>(
             comparisonBranch(stop).getTrueDest()->back())
             .getDest() == exit);
  EXPECT(mlir::cast<mlir::cf::BranchOp>(
             comparisonBranch(stop).getFalseDest()->back())
             .getDest() == increment);
  EXPECT(mlir::isa<mlir::func::ReturnOp>(exit->back()));

  unsigned initializations = 0;
  function.walk([&](tensorium::StoreOp store) {
    if (store.getAddress() != counter.getAddress() ||
        !store->getOperand(0).getDefiningOp<mlir::arith::ConstantIntOp>())
      return;
    ++initializations;
    EXPECT(store->getBlock() == &function.getBody().front());
  });
  EXPECT(initializations == 1);
}

void lowers_for_with_omitted_clauses() {
  Context c("int finite(int n) { for (; n > 0;) n = n - 1; return n; } "
            "int infinite(void) { for (;;) break; return 7; }");
  auto module = c.generate();
  auto finite = module->lookupSymbol<mlir::func::FuncOp>("finite");
  auto *condition =
      mlir::cast<mlir::cf::BranchOp>(finite.getBody().front().back()).getDest();
  auto test = mlir::cast<mlir::cf::CondBranchOp>(condition->back());
  auto *increment =
      mlir::cast<mlir::cf::BranchOp>(test.getTrueDest()->back()).getDest();
  EXPECT(increment->getOperations().size() == 1);
  EXPECT(mlir::cast<mlir::cf::BranchOp>(increment->back()).getDest() ==
         condition);
  EXPECT(mlir::isa<mlir::func::ReturnOp>(test.getFalseDest()->back()));

  auto infinite = module->lookupSymbol<mlir::func::FuncOp>("infinite");
  auto *always =
      mlir::cast<mlir::cf::BranchOp>(infinite.getBody().front().back())
          .getDest();
  EXPECT(always->getOperations().size() == 1);
  auto *body = mlir::cast<mlir::cf::BranchOp>(always->back()).getDest();
  EXPECT(body->getOperations().size() == 1);
  auto *exit = mlir::cast<mlir::cf::BranchOp>(body->back()).getDest();
  EXPECT(exit != always && exit != body);
  EXPECT(mlir::isa<mlir::func::ReturnOp>(exit->back()));
}

void preserves_for_initializer_condition_and_void_increment_calls() {
  Context c("void start(void); int ready(void); void step(void); "
            "void f(void) { for (start(); ready(); step()) continue; }");
  auto module = c.generate();
  auto function = module->lookupSymbol<mlir::func::FuncOp>("f");
  mlir::func::CallOp start, ready, step;
  function.walk([&](mlir::func::CallOp call) {
    if (call.getCallee() == "start") {
      EXPECT(!start);
      start = call;
    } else if (call.getCallee() == "ready") {
      EXPECT(!ready);
      ready = call;
    } else if (call.getCallee() == "step") {
      EXPECT(!step);
      step = call;
    }
  });
  EXPECT(start && ready && step);
  EXPECT(start->getBlock() == &function.getBody().front());
  auto *condition =
      mlir::cast<mlir::cf::BranchOp>(start->getBlock()->back()).getDest();
  EXPECT(ready->getBlock() == condition);
  auto test = mlir::cast<mlir::cf::CondBranchOp>(condition->back());
  EXPECT(mlir::cast<mlir::cf::BranchOp>(test.getTrueDest()->back()).getDest() ==
         step->getBlock());
  EXPECT(step.getNumResults() == 0);
  EXPECT(mlir::cast<mlir::cf::BranchOp>(step->getBlock()->back()).getDest() ==
         condition);
  EXPECT(mlir::isa<mlir::func::ReturnOp>(test.getFalseDest()->back()));
}

void keeps_nested_for_jump_targets_separate() {
  Context c("int f(int n) { for (int i = 0; i < n; i = i + 1) { "
            "for (int j = 0; j < n; j = j + 1) { "
            "if (j == 1) continue; break; } "
            "if (i == 2) continue; break; } return 0; }");
  auto module = c.generate();
  auto function = module->lookupSymbol<mlir::func::FuncOp>("f");
  std::vector<mlir::arith::CmpIOp> loops, skips;
  function.walk([&](mlir::arith::CmpIOp comparison) {
    if (comparison.getPredicate() == mlir::arith::CmpIPredicate::slt)
      loops.push_back(comparison);
    if (comparison.getPredicate() == mlir::arith::CmpIPredicate::eq)
      skips.push_back(comparison);
  });
  EXPECT(loops.size() == 2 && skips.size() == 2);
  for (auto skip : skips) {
    auto counter = skip.getLhs().getDefiningOp<tensorium::LoadOp>();
    EXPECT(counter);
    mlir::arith::CmpIOp loop;
    for (auto candidate : loops) {
      auto load = candidate.getLhs().getDefiningOp<tensorium::LoadOp>();
      if (load && load.getAddress() == counter.getAddress())
        loop = candidate;
    }
    EXPECT(loop);
    auto *increment = forIncrement(function, counter.getAddress());
    auto test = comparisonBranch(skip);
    EXPECT(
        mlir::cast<mlir::cf::BranchOp>(test.getTrueDest()->back()).getDest() ==
        increment);
    EXPECT(
        mlir::cast<mlir::cf::BranchOp>(test.getFalseDest()->back()).getDest() ==
        comparisonBranch(loop).getFalseDest());
    EXPECT(mlir::cast<mlir::cf::BranchOp>(increment->back()).getDest() ==
           loop->getBlock());
  }
}

void preserves_for_initializer_shadowing_and_body_returns() {
  Context c("int f(int i) { for (int i = 0; i < 2; i = i + 1) { "
            "if (i == 1) return i; } return i; }");
  auto module = c.generate();
  auto function = module->lookupSymbol<mlir::func::FuncOp>("f");
  mlir::Value parameterStorage, counterStorage;
  function.walk([&](tensorium::StoreOp store) {
    if (store->getOperand(0) == function.getArgument(0))
      parameterStorage = store.getAddress();
  });
  function.walk([&](mlir::arith::CmpIOp comparison) {
    if (comparison.getPredicate() == mlir::arith::CmpIPredicate::slt)
      counterStorage =
          comparison.getLhs().getDefiningOp<tensorium::LoadOp>().getAddress();
  });
  EXPECT(parameterStorage && counterStorage);
  EXPECT(parameterStorage != counterStorage);
  unsigned parameterReturns = 0, counterReturns = 0;
  function.walk([&](mlir::func::ReturnOp ret) {
    auto load = ret.getOperands().front().getDefiningOp<tensorium::LoadOp>();
    EXPECT(load);
    if (load.getAddress() == parameterStorage)
      ++parameterReturns;
    else if (load.getAddress() == counterStorage)
      ++counterReturns;
    else
      EXPECT(false);
  });
  EXPECT(parameterReturns == 1 && counterReturns == 1);
}

void rejects_unsupported_constructs() {
  struct Case {
    const char *source;
    const char *message;
  };
  const Case cases[] = {
      {"int f(int x) { return x / 2; }",
       "does not support this binary operator"},
      {"int f(float a, float b) { return a + b; }",
       "does not support this implicit conversion"},
      {"int f(volatile int x) { return x; }", "volatile parameter storage"},
      {"int f(int x) { return x += 1; }",
       "does not support this binary operator"},
      {"int f(int n) { for (n / 2; n; n = n - 1) {} return n; }",
       "does not support this binary operator"},
      {"int f(int n) { for (; n / 2; n = n - 1) {} return n; }",
       "does not support this binary operator"},
      {"int f(int n) { for (; n; n += 1) {} return n; }",
       "does not support this binary operator"},
      {"int global;", "not implemented for this declaration"},
      {"int f(void) { static int x; return x; }",
       "automatic scalar local storage"},
      {"int f(void) { int x = 1; }",
       "requires a return in a non-void function"},
      {"int f(void) { volatile int x = 1; return x; }",
       "automatic scalar local storage"},
  };
  for (auto &test : cases) {
    Context c(test.source);
    EXPECT(!c.generator.generate(*c.unit));
    EXPECT(c.diag.errorCount() == 1);
    if (c.output.str().find(test.message) == std::string::npos)
      throw std::runtime_error(c.output.str());
  }
}

int main() {
  struct Test {
    const char *name;
    void (*run)();
  };
  const Test tests[] = {
      {"maps_parameters_from_declarations", maps_parameters_from_declarations},
      {"lowers_nested_binary_expressions", lowers_nested_binary_expressions},
      {"keeps_parameter_bindings_local_to_each_function",
       keeps_parameter_bindings_local_to_each_function},
      {"lowers_integer_promotions", lowers_integer_promotions},
      {"lowers_integer_to_bool_conversions",
       lowers_integer_to_bool_conversions},
      {"lowers_target_dependent_integer_conversions",
       lowers_target_dependent_integer_conversions},
      {"lowers_identity_constants_and_private_functions",
       lowers_identity_constants_and_private_functions},
      {"lowers_locals_and_discarded_calls", lowers_locals_and_discarded_calls},
      {"respects_local_shadowing_and_mutation",
       respects_local_shadowing_and_mutation},
      {"shares_function_symbols_across_prototypes_and_definitions",
       shares_function_symbols_across_prototypes_and_definitions},
      {"supports_external_void_nested_and_recursive_calls",
       supports_external_void_nested_and_recursive_calls},
      {"converts_local_initializers_and_preserves_explicit_returns",
       converts_local_initializers_and_preserves_explicit_returns},
      {"shares_parameter_storage_across_nested_control_flow",
       shares_parameter_storage_across_nested_control_flow},
      {"routes_for_continue_through_increment_and_break_to_exit",
       routes_for_continue_through_increment_and_break_to_exit},
      {"lowers_for_with_omitted_clauses", lowers_for_with_omitted_clauses},
      {"preserves_for_initializer_condition_and_void_increment_calls",
       preserves_for_initializer_condition_and_void_increment_calls},
      {"keeps_nested_for_jump_targets_separate",
       keeps_nested_for_jump_targets_separate},
      {"preserves_for_initializer_shadowing_and_body_returns",
       preserves_for_initializer_shadowing_and_body_returns},
      {"rejects_unsupported_constructs", rejects_unsupported_constructs},
  };
  int failed = 0;
  for (auto &test : tests) {
    try {
      test.run();
      std::cout << "[PASSED] " << test.name << '\n';
    } catch (const std::exception &e) {
      ++failed;
      std::cerr << "[FAILED] " << test.name << ": " << e.what() << '\n';
    }
  }
  return failed ? 1 : 0;
}
