# C object memory

CC1 emits Tensorium memory operations and typed pointers. General C object
memory uses the `tensorium_to_llvm` library and the
`--convert-tensorium-to-llvm` pass in `tensorium-opt`. This pass also lowers
`arith`, `cf` and `func`, yielding the LLVM dialect directly.

## Supported subset

- Automatic integer/floating-point object storage and fixed-size array storage.
- Local pointers, pointers to pointers, address-taking and reassignment of
  pointer parameters, and pointer results from functions.
- Array decay, indexing (including `i[p]`), arrays of pointers, multidimensional
  arrays, and pointers to whole fixed-size array rows.
- Pointer addition/subtraction by an integer, `+=`/`-=` on pointers, pointer
  differences in elements, equality and relational pointer comparisons.
- Null pointer constants, pointer conditions, logical not, pointer and integer
  pre/post increment/decrement, integer `+=`/`-=`/`*=`, and `sizeof` for supported
  complete objects. Compound assignments evaluate their address once, compute
  in the promoted type and convert the stored/result value back to the object type.
- Ordinary native pointer parameters and results at external function calls.

Floating-point storage is supported by the dialect and lowering; the existing
frontend's floating-point expressions/conversions remain limited. Division,
remainder and bitwise/shift compound assignments retain their previous limits.
Array bounds must currently be positive integer literals, as required by the
parser. Arrays are filled by assignments; brace/string aggregate initializers
and variable-length arrays are not implemented. Neither are structs/unions,
global/static object generation, function pointers, general pointer casts,
`void *`, volatile or atomic accesses, or protected pointers.
This milestone does not claim complete C memory support.

Ordinary `malloc`/`free` and memory-library prototypes are currently blocked by
their `void *` types. Typed external allocation/release wrappers can already be
compiled and used; the compiler does not need its own allocator for that path.
Heap allocation tracking and protection metadata are not implemented.

## SSA bindings and automatic storage

`LocalStorageAnalysis` runs on the typed AST before a function is emitted. A
scalar parameter can stay in SSA when it is neither modified nor addressed. An
automatic scalar local can stay in SSA when it has an initializer, is neither
modified nor addressed, is non-volatile and does not refer to itself while its
initializer is evaluated. Unsupported AST nodes conservatively require storage.
The backend's existing type and volatile-access restrictions still apply.

For example, the generator binds `y` directly to its initializer result:

```c
int f(int x) {
  int y = x + 1;
  return y * 2;
}
```

This function needs no Tensorium allocation, load or store. The choice is made
during generation, without relying on a later promotion or cleanup pass.
Read-only bindings remain SSA values in `if`, `while` and `for` bodies.

A pointer variable can also be an SSA value while the pointed-to object remains
mutable. Writing through `*p` or `p[i]` does not itself modify `p`; rebinding or
incrementing `p` does. Pointee loads and stores remain real memory accesses, and
copying a loaded value into a local preserves the value read at initialization.
For the current fixed-size type subset, `sizeof` does not evaluate its operand,
so `sizeof(x++)` and `sizeof(&x)` do not require storage for an otherwise
unmodified, non-addressed `x`.

Modified or address-taken parameters are allocated and initialized in the
function entry block. Other automatic objects that require storage, including
fixed-size arrays and mutable loop locals, also receive entry-block allocations.
Their initializers remain at the source declaration: they run on each relevant
iteration, after any preceding conditions, and are skipped when control flow
does not reach the declaration. Uninitialized locals receive no implicit store.
Different declarations keep distinct allocations, including shadowed objects
whose lifetimes overlap.

This placement reserves storage once per function invocation. It does not add
lexical lifetime markers, protected allocation identities or dynamic stack
save/restore. VLAs remain unsupported. Hand-written Tensorium IR may still put
an `alloca` inside a loop; the operation retains its per-execution semantics and
the LLVM conversion does not perform this frontend placement policy.

Global initializer expressions can now be parsed and type-checked, but global
constant-expression checks and object generation are still in progress. This
does not enable global storage in `--emit-mlir`. See
[the frontend status](sema.md#file-scope-objects-in-progress).

## IR and representation

`!tensorium.array<N, T>` describes a contiguous memory object, not an SSA tensor.
Arrays may contain scalar objects, typed pointers or other arrays. `N` must be
positive. A pointer to an array is distinct from a pointer to its first element.
`tensorium.array_decay` verifies that relationship and derives the latter.

`tensorium.alloca`, `load` and `store` handle pointer-valued storage as well as
scalar objects. `tensorium.ptr_offset` advances by a signed number of pointee
objects, using the complete object stride, including array rows. CC1 extends
signed/unsigned indices appropriately to the compilation target's pointer width
before emitting offsets. The lowering does not add LLVM `inbounds` promises.

`tensorium.null` produces a typed null. `tensorium.ptr_cmp` supports `eq`, `ne`,
`ult`, `ule`, `ugt`, `uge` and requires matching pointer types.
`tensorium.ptr_diff` computes a signed difference in elements, requiring pointers
into the same array object and a representable result. Its lowering computes
the byte difference and divides by the target allocation stride, before any
result-width conversion.

The direct conversion maps typed pointers to ordinary opaque LLVM pointers and
arrays recursively to LLVM array types. This supports storing a pointer in a
pointer-sized slot and preserves the ordinary pointer ABI at calls/returns.
Allocation, load and store use ABI alignment supplied by LLVM for the converted
object type. Each allocation execution creates an object whose storage lasts
until the enclosing automatic allocation scope ends.

## Target contract

CC1 records its target in module attributes `llvm.data_layout` and
`llvm.target_triple`. The conversion requires a valid explicit data layout and
derives index width and object allocation stride from it. It does not infer the
target from the machine running the compiler. `sizeof` uses the frontend's
matching target sizes for the supported object types and does not evaluate its
operand. Tests cover 32-bit and 64-bit LLVM layouts independently of the host.

The layout requirement also applies to hand-written or previously saved modules
that contain only standard operations. For example, the current AArch64/macOS
preset uses:

```mlir
module attributes {
  llvm.data_layout = "e-m:o-i64:64-i128:128-n32:64-S128",
  llvm.target_triple = "aarch64-apple-macosx13.0.0"
} {
}
```

Use the layout of the intended target rather than copying this preset for a
different platform. Fresh output from `cc1 --emit-mlir` includes the target
attributes automatically; standard canonicalization preserves them.

This is an unprotected lowering. It adds no bounds, lifetime or null checks.
The usual C rules for valid accesses, one-past pointers, pointer differences and
object lifetime apply. Future MemProtect passes must run before pointer types
and derivation are erased by conversion to LLVM.

## Compatibility with MemRef

`--convert-tensorium-to-memref` remains available for its original scalar
subset. It uses rank-zero MemRef descriptors and intentionally rejects nested
pointer storage and external pointer declarations. General C pointer variables
are not represented using nested MemRefs. Native scalar compatibility tests
continue to exercise that path independently of the direct LLVM path.

## Build and execute

```sh
cmake --build "$HOME/Developer/build/tensorium-llvm24" -j 2 &&
ctest --test-dir "$HOME/Developer/build/tensorium-llvm24" \
  -R 'mlir_memory_|tensorium_to_llvm_|mlir_codegen|local_storage_analysis|mlir_local_ssa|mlir_parameter_ssa_runtime|mlir_ssa_' \
  --output-on-failure
```

The C fixtures under `Tests/CodeGen/Memory/valid` are compiled and run at `-O0`
and `-O2`. The external-call fixture links a separate Clang-compiled translation
unit, exercising both directions of the pointer ABI. Conversion fixtures cover
layouts, parse/print round-trips, idempotence, verifier failures, invalid target
layouts and rejection of residual Tensorium types hidden in metadata.

`Tests/CodeGen/SSA` adds raw-IR checks and native fixtures for read-only bindings,
mutable fallbacks, entry-block allocations and loop initializer evaluation.
Its analysis-only fixture is never generated or executed. See
[the SSA test guide](../Tests/CodeGen/SSA/README.md) for individual test names and
commands. A registered test describes coverage; consult the current CTest run
for its validation result.

## Generation audit

The [memory and CodeGen audit](audits/memory_codegen_audit.md) records actual
generated IR, remaining frontend/ABI gaps, redundant memory traffic, and
the original SSA/storage proposal. Its measurements describe the older compiler
baseline, before SSA binding selection and entry-block allocation placement.
They have not been remeasured for the current generator. Lexical lifetime,
general mutable-variable SSA construction and protected-memory work remain open.
