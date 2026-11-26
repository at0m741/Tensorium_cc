# C memory tests

CTest registers C programs with `main` from `valid/`. They cover loads and stores,
aliasing, local and nested pointers, reassignment in branches and loops,
arrays, arrays of pointers, multidimensional arrays, signed offsets,
element-based differences, increments, null pointers and unevaluated `sizeof`
operands. Compound integer assignments check promotion, narrowing, result values
and evaluation of the left-hand address exactly once.

`pointer_external.c` uses `PointerExternalRuntime.c`, compiled separately by
Clang. Calls cross the boundary in both directions and check pointer arguments,
pointer results and writes through `int **`.

`void_pointer.c` exercises erasure/recovery of object pointers, void-pointer
storage, globals and qualifier-preserving conversions. `void_pointer_external.c`
and its native helper test both directions of calls and full-width integer
results. `integer_returns.c` covers the supported signed/unsigned return types.

`invalid/` retains void-pointer arithmetic/dereference, qualifier-discarding
conversions, narrow integer returns awaiting ABI attributes, volatile accesses
and writes through `const`. Previous restrictions on pointer-valued storage
are now covered by valid programs that execute as native code.

CTest checks generation and diagnostics for every fixture. When LLVM tools
and Clang are available, valid programs use this pipeline:

```text
cc1 -> Tensorium -> LLVM dialect -> LLVM IR -> Clang -> execution
```

Each program executes at `-O0` and `-O2`. A nonzero exit code identifies the
failed check. Intermediate files remain under `Tests/CodeGen/Memory/*-runtime/`
in the build directory.

The separate [SSA and loop storage suite](../SSA/README.md) checks read-only
bindings, fixed-size entry-block allocations and initializer placement. These
checks inspect raw generated IR before any cleanup pass and include native
programs with nested loops, arrays, aliasing, `continue`/`break` and zero-trip
initializer cases.

The scalar tests in `Tests/Conversion/scalar_memory.mlir` continue to exercise
the Tensorium -> MemRef -> LLVM compatibility path. The new conversion is also
tested with 32-bit and 64-bit pointer layouts in `Tests/Conversion/LLVM/`.

```sh
ctest --test-dir "$HOME/Developer/build/tensorium-llvm24" \
  -R 'mlir_memory_|tensorium_to_llvm_' --output-on-failure
```
