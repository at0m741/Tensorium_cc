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
cmake --build "$HOME/Developer/build/tensorium-llvm24" -j 2
ctest --test-dir "$HOME/Developer/build/tensorium-llvm24" \
  -R 'mlir_memory_|tensorium_to_llvm_' --output-on-failure
```

The C fixtures under `Tests/CodeGen/Memory/valid` are compiled and run at `-O0`
and `-O2`. The external-call fixture links a separate Clang-compiled translation
unit, exercising both directions of the pointer ABI. Conversion fixtures cover
layouts, parse/print round-trips, idempotence, verifier failures, invalid target
layouts and rejection of residual Tensorium types hidden in metadata.

## Generation audit

The [memory and CodeGen audit](audits/memory_codegen_audit.md) records actual
generated IR, remaining frontend/ABI gaps, redundant memory traffic, and a
proposed SSA/storage split. It also documents loop-local stack allocation and
lexical lifetime work that remains necessary before a protected pipeline.
