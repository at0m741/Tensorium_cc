# SSA bindings and loop storage tests

This suite checks the frontend's choice between SSA values and object storage.
It also checks that fixed-size allocations execute once per function invocation
while local initializers retain their source execution frequency. The C inputs
are real files, shared by the raw-IR checks and native execution tests.

## Fixtures

| File | Coverage |
| --- | --- |
| [parameters.c](parameters.c) | Read-only parameter bindings across branches and loops; modified, addressed and rebound parameters; unevaluated `sizeof` operands. |
| [locals.c](locals.c) | Initialized scalar and pointer bindings, dependent locals, narrowing/sign extension, captured pointee values and single evaluation of initializer calls. |
| [locals_control_flow.c](locals_control_flow.c) | Branch-local values, shadowing, loop temporaries, read-only `for` initializer bindings and `continue`/`break`. |
| [locals_memory.c](locals_memory.c) | Memory fallbacks for modified, uninitialized and addressed locals; pointer aliases/rebinding; arrays and copies of mutable values. |
| [loop_storage.c](loop_storage.c) | Entry-block allocations for mutable/addressed locals and arrays, nested loops with overlapping shadowed objects, repeated initializers and zero-trip loops. |
| [locals_analysis_only.c](locals_analysis_only.c) | Conservative storage decisions for self-references, volatile objects, `static` locals and `extern` declarations. This file is analyzed only, never generated or executed. |

The analysis-only cases intentionally include self-initializers with
indeterminate reads and storage kinds not yet supported by CodeGen. Keeping
them outside the native runtime list avoids executing those cases.

## Structural checks

[`LocalSSATests.cpp`](LocalSSATests.cpp) parses and semantically analyzes the
fixtures, then verifies supported generated modules without running optimization
passes. It checks:

- No allocation, store or load for eligible scalar bindings.
- Retained pointee accesses and snapshot values when pointer variables stay in
  SSA; an SSA pointer does not make the referenced memory immutable.
- Initializer calls evaluated once per execution of their declaration, with
  repeated local reads using the same result.
- The expected memory fallbacks for mutable, addressed and aggregate objects.
- All fixed-size allocations in the function entry block, including objects
  declared in nested loops.
- Initializer calls and pointee loads outside the entry block, with the store of
  an initializer result at the call's control-flow position.

The executable is `cc1_local_ssa_tests`; CTest registers it as `mlir_local_ssa`.
The related `local_storage_analysis` test uses
[`LocalStorageAnalysisTests.cpp`](../LocalStorageAnalysisTests.cpp) and its
[typed C fixture](../LocalStorage/parameters.c). The older `mlir_codegen` test
also checks parameter SSA bindings and storage behavior.

## Native checks

When the selected LLVM/MLIR tools and Clang are available, CTest registers:

- `mlir_parameter_ssa_runtime`
- `mlir_ssa_locals_runtime`
- `mlir_ssa_locals_control_flow_runtime`
- `mlir_ssa_locals_memory_runtime`
- `mlir_ssa_loop_storage_runtime`

Each runtime case uses
[`CheckMLIRForRuntime.cmake`](../../CheckMLIRForRuntime.cmake) to generate raw
Tensorium IR, lower it to the LLVM dialect, translate it to LLVM IR, compile with
Clang and execute at both `-O0` and `-O2`. A nonzero exit status identifies the
failed source check. Artifacts remain in this directory's `*-runtime/` folders
inside the configured build tree.

The loop storage fixture checks zero and multiple iterations, distinct live
objects in nested scopes, and initialization after `continue`/`break` conditions.
Its zero-trip pointer case passes a null pointer to a loop whose body must not
execute; a misplaced dereferencing initializer would change the behavior.

## Build and run

Using the LLVM 24 build directory:

```sh
cmake --build "$HOME/Developer/build/tensorium-llvm24" -j 2 &&
ctest --test-dir "$HOME/Developer/build/tensorium-llvm24" \
  -R '^(mlir_codegen|local_storage_analysis|mlir_local_ssa|mlir_parameter_ssa_runtime|mlir_ssa_.*_runtime)$' \
  --output-on-failure
```

Run only the loop storage checks with:

```sh
ctest --test-dir "$HOME/Developer/build/tensorium-llvm24" \
  -R '^(mlir_local_ssa|mlir_ssa_loop_storage_runtime)$' --output-on-failure
```

Run CTest without `-R` for the full configured suite. These commands describe
how to validate the current checkout; they do not assert a recorded passing
test count.

## Scope

The frontend currently uses memory for mutable locals rather than constructing
general SSA joins. Entry-block storage placement applies to supported fixed-size
automatic objects; it does not implement VLAs, lexical lifetime markers or
memory-protection metadata. Global-object generation is a separate work in
progress. See [the memory contract](../../../docs/memory.md) and
[the global frontend status](../../../docs/sema.md#file-scope-objects-in-progress).
