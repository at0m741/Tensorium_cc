# Global objects and declarative IR

File-scope objects are emitted through the Tensorium dialect before LLVM
lowering. This document describes the implemented subset, not C99 conformance.

## Source layout

```text
include/tensorium/Dialect/Tensorium/IR/
  TensoriumOps.td          # Shared operation base and includes
  TensoriumMemoryOps.td    # Allocation, accesses and pointer operations
  TensoriumGlobalOps.td    # Global definitions/declarations and addresses
  TensoriumEnums.td        # Pointer predicates and object linkage
  TensoriumEnums.h         # Public generated enum declarations
include/cc1/AST/ConstantExpr.hpp
lib/AST/ConstantExpr.cpp
lib/CodeGen/MLIRGenGlobals.cpp
Tests/CodeGen/Globals/
Tests/Conversion/LLVM/
```

ODS generates operation classes, builders, accessors, assembly parsers/printers
and structural verification. Global symbol resolution and initializer/type
relationships use C++ verifiers. Native conversion remains in
`TensoriumToLLVM.cpp`. Neither the AST nor Sema depends on MLIR.

## Supported subset

- Compatible file-scope declarations and tentative definitions share one symbol.
  An initialized definition takes precedence over tentative definitions and
  `extern` declarations. Duplicate initialized definitions are diagnosed.
- Internal (`static`) and external object linkage are preserved independently
  of MLIR symbol visibility. Tentative definitions become zero definitions in
  their translation unit, using strong external linkage rather than common symbols.
- Integer initializers support typed literals, integral arithmetic conversions,
  unary `+`/`-`, and binary `+`, `-`, `*`. Evaluation uses target widths, unsigned
  wraparound and checked signed arithmetic. Unsupported expressions are rejected.
- Supported scalar, pointer and complete fixed-array objects can be
  zero-initialized. A pointer initializer converted from a null pointer constant
  is also supported. Const objects carry the constant marker.
- Loads, stores, address-taking and array access use `tensorium.get_global`.
  Automatic locals continue to use declaration-specific SSA/storage decisions.
- External objects can be resolved by separately compiled native C code.

Example raw IR:

```mlir
module attributes {llvm.data_layout = "e-p:64:64"} {
  tensorium.global external @counter : i32 {initial_value = 3 : i32}
  tensorium.global internal @zero : i32 {initial_value = unit}
  tensorium.global external @provided_elsewhere : i32
  func.func @read() -> i32 {
    %p = tensorium.get_global @counter : !tensorium.ptr<i32>
    %v = tensorium.load %p : !tensorium.ptr<i32>
    return %v : i32
  }
}
```

Pointer comparisons now use generated `PtrPredicate` enum attributes:
`tensorium.ptr_cmp ne %a, %b : !tensorium.ptr<i32>`. The generic representation
stores a validated integer enum attribute rather than a free-form string.
Old generic IR containing a string-valued predicate must be updated.

## Remaining work

Complete C99 static-initializer legality checking in Sema is still missing.
The current evaluator is called by CodeGen, so semantic-only checking can accept
an initializer that emission rejects. Unsupported cases include other constant
operators and `sizeof`, non-null address relocations, floating initializers,
aggregate/string initializers, volatile accesses and static local storage.
The CLI still handles one translation unit and does not drive native linking.

## Validation

The C fixtures test merging declarations, linkage, local shadowing, zero storage,
integer arithmetic, external native linkage and rejection diagnostics. Structural
checks verify that repeated declarations produce exactly two globals in the
declaration fixture and introduce no automatic allocation. MLIR fixtures cover
symbol/type errors, initializers, enum validation and LLVM conversion round trips.
AST-level boundary tests cover target-dependent `long` widths, signed overflow,
unsigned wraparound and sign extension without depending on MLIR.
Runtime fixtures compile and execute at `-O0` and `-O2` when the tools are available.

```sh
cmake --build "$HOME/Developer/build/tensorium-llvm24" -j 2 &&
ctest --test-dir "$HOME/Developer/build/tensorium-llvm24" \
  -R 'mlir_global_|tensorium_to_llvm_|mlir_codegen|mlir_cli|sema|ast_constant_expr' \
  --output-on-failure
```
