# Memory support and CodeGen audit

Date: 2026-11-19. Compiler baseline: `5892db6c8a78bf0a87306c1e2803ce2c8a2eea8f`.
Inspection and experiments used the installed LLVM/MLIR 24.0.0git package.
Native checks used the installed Clang 20 at both `-O0` and `-O2`.

This is an audit, not a compiler implementation change. The reproducible C
inputs are in [memory_codegen_cases.c](memory_codegen_cases.c). Counts below
are static operation counts, not execution counts or performance measurements.

## Remaining memory-related features

The following legal C inputs were tested separately through parsing
(`--dump-ast`), semantic analysis (`--dump-sema`) and CodeGen (`--emit-mlir`).
The table records the first blocking phase in this compiler.

| Feature | Example | First blocker |
| --- | --- | --- |
| Generic object pointers | `int f(void *p) { int *q = p; return *q; }` | CodeGen rejects `void *`. |
| Explicit pointer casts | `*((int *)p)` | Parser rejects the cast expression. |
| Pointer/integer casts | `(long)p` | Parser rejects the cast expression. |
| Global and external objects | `int global;` or `extern int global;` | CodeGen rejects the object declaration. |
| Static local storage | `static int count = 0;` | CodeGen rejects the storage class. |
| Thread-local storage | `_Thread_local int x;` | Parser rejects the declaration. |
| Struct and union definitions | `struct Pair { int x; int y; };` | Parser reports definitions as unimplemented. Layout, field access, aggregate ABI and bit-fields also need an implementation. |
| Aggregate initialization | `int a[2] = {1, 2};` | Parser rejects the initializer. |
| String storage | `char *p = "abc";` | CodeGen cannot obtain storage for the string literal. |
| Computed constant bounds | `int a[2 + 2];` | Parser accepts only integer literal bounds. |
| Variable-length arrays | `int a[n];` | Same parser restriction; dynamic extent and scope restoration also need an IR/lowering design. |
| Volatile memory | `int f(volatile int *p) { return *p; }` | CodeGen explicitly rejects volatile pointees. |
| Atomic memory | `_Atomic(int) *p` | Parser rejects the parameter type; atomic ordering and access operations are absent. |
| Explicit alignment | `_Alignas(16) int x;` | Parser does not handle the declaration. |
| Alias contracts | `int *restrict p` | Parser rejects the qualifier; the type model has no restrict contract. |
| Function pointers | `int f(int (*p)(int)) { return p(2); }` | CodeGen rejects the function-pointer parameter. Indirect calls also remain unsupported. |
| Conditional pointer expressions | `return c ? p : q;` | CodeGen rejects the conditional expression. |
| Short-circuit pointer guards | `return p && *p;` | CodeGen rejects logical AND. Lazy evaluation is required when implementing it. |
| Broader function ABI | A declaration returning `size_t` | CodeGen currently accepts only `int`, `void` and pointer results; variadic declarations are also rejected. |

The last three rows are broader frontend/ABI gaps that affect otherwise useful
memory code; they should not be confused with missing load/store primitives.

### Heap and library calls

The normal `malloc`/`free` and `memcpy` prototypes currently fail because of
their `void *` types. This does not mean that all heap use is impossible:
CodeGen accepts typed external allocation and release wrappers, and a native
test successfully allocated, wrote, read and freed an object through such
wrappers. The compiler does not need to implement its own allocator to compile
ordinary library calls. It needs compatible pointer types/casts and the
corresponding call ABI. A protected pipeline would additionally need allocation
and release instrumentation and runtime metadata.

### Object lifetime and protection

The current allocation contract releases storage at the nearest enclosing
`AutomaticAllocationScope`, which is the function in these C examples. The
generator does not emit lexical block lifetime boundaries, stack save/restore,
or explicit object-generation metadata. Stack storage management and logical C
object lifetime need separate treatment, especially for loop-local objects,
future VLAs and temporal memory protection.

Bounds, null, use-after-free and use-after-scope checks are not implemented.
Allocation identity, bounds, runtime allocation type and lifetime information
needed by a protected pipeline are not yet fully represented. Most generated
operations also use unknown source locations, which future diagnostics cannot
reconstruct after lowering.

## Generated memory traffic

`A / S / L` means allocation, store and load operation counts. The final column
uses `--convert-tensorium-to-llvm --sroa --mem2reg --canonicalize --cse`.
These are separate inspections of the same input, not equivalent optimization
levels built into CC1.

| Function | Raw Tensorium A/S/L | Tensorium canonicalize + CSE A/S/L | LLVM after promotion A/S/L |
| --- | --- | --- | --- |
| `straightline` | 1 / 1 / 1 | 1 / 1 / 1 | 0 / 0 / 0 |
| `reassign` | 1 / 3 / 3 | 1 / 3 / 3 | 0 / 0 / 0 |
| `readonly_branch` | 2 / 2 / 3 | 2 / 2 / 3 | 0 / 0 / 0 |
| `merge` | 3 / 5 / 4 | 3 / 5 / 4 | 0 / 0 / 0 |
| `pointer_local` | 1 / 1 / 2 | 1 / 1 / 2 | 0 / 0 / 1 |
| `address_local` | 1 / 1 / 1 | 1 / 1 / 1 | 0 / 0 / 0 |
| `alias_call` | 1 / 1 / 1 | 1 / 1 / 1 | 1 / 1 / 1 |
| `alias_store` | 1 / 2 / 3 | 1 / 2 / 3 | 0 / 1 / 2 |
| `boolean_branch` | 1 / 1 / 1 | 1 / 1 / 1 | 0 / 0 / 0 |
| `comparison_value` | 0 / 0 / 0 | 0 / 0 / 0 | 0 / 0 / 0 |
| `repeated_index` | 0 / 1 / 2 | 0 / 1 / 2 | 0 / 1 / 2 |
| `repeated_array` | 1 / 1 / 2 | 1 / 1 / 1 | 1 / 1 / 1 |
| `loop_temp` | 4 / 6 / 7 | 4 / 6 / 7 | 1 / 1 / 1 |
| `pointer_difference` | 0 / 0 / 0 | 0 / 0 / 0 | 0 / 0 / 0 |

Running `--mem2reg` directly on the Tensorium input left the output unchanged,
byte for byte. Tensorium's memory operations currently provide effect
interfaces, but do not implement the memory-slot promotion interfaces used by
that pass. LLVM allocation/load/store operations do provide those interfaces.

## Changes that belong in generation

### 1. Keep unobservable scalar variables in SSA

[MLIRGenStmt.cpp](../../lib/CodeGen/MLIRGenStmt.cpp) creates object storage for
every automatic local declaration. Thus:

```c
int straightline(int x) {
  int y = x + 1;
  return y * 2;
}
```

currently produces:

```mlir
func.func @straightline(%arg0: i32) -> i32 {
  %0 = tensorium.alloca : <i32>
  %c1_i32 = arith.constant 1 : i32
  %1 = arith.addi %arg0, %c1_i32 : i32
  tensorium.store %1, %0 : <i32>
  %2 = tensorium.load %0 : <i32>
  %c2_i32 = arith.constant 2 : i32
  %3 = arith.muli %2, %c2_i32 : i32
  return %3 : i32
}
```

The local `y` can instead bind directly to the `arith.addi` result, eliminating
all three memory operations at generation time. SSA value names are normal;
the unnecessary conversion between values and memory is the issue.

A pointer variable can likewise hold a `!tensorium.ptr<T>` SSA value without
having its own pointer-sized slot. That does not eliminate accesses to the
pointed-to object: `pointer_local` still needs one real load through its pointer.

### 2. Stop spilling read-only parameters at control-flow boundaries

`materializeParameters()` in
[MLIRGenMemory.cpp](../../lib/CodeGen/MLIRGenMemory.cpp) creates storage for
every remaining parameter. It is called by each `if`, `while` and `for` emitter.
In `readonly_branch`, neither parameter is assigned or addressed, yet both
are spilled and reloaded. Entry block arguments already dominate their uses.

A per-function declaration-use analysis can identify writes, address-taking,
aggregates and observable accesses before emission. Read-only, non-addressed
parameters can remain SSA values throughout the function. Writing through `p`
does not itself modify the pointer variable `p`.

### 3. Remove loop-local non-addressed allocations before relying on cleanup

`loop_temp` creates the storage for `temp` inside its loop body. One allocation,
one store and one load remain there even after the tested LLVM promotion
pipeline. At native `-O0`, the generated AArch64 assembly subtracts another
16 bytes from the stack pointer on each iteration and restores the stack at
function exit. This is an execution-frequency issue, not just an extra SSA name.

The downstream native `-O2` optimizer removes the loop and stack traffic for
this specific arithmetic example. That does not repair the earlier Tensorium
IR consumed by analyses/protection passes, or the native `-O0` path.

An initialized, non-addressed, unmodified `temp` should be an SSA value in its
definition block. Do not blindly hoist every allocation: observable addresses,
allocation identity, lexical lifetime and dynamic extents require their own
rules. Uninitialized and more complex cases can retain a memory fallback while
SSA construction is being extended.

### 4. Generate conditions as `i1`

[MLIRGenBinary.cpp](../../lib/CodeGen/MLIRGenBinary.cpp) extends comparison
results to C's `int` result type. `emitCondition` then compares that `i32` value
against zero. For `if (x < 10)`, the raw pattern is:

```mlir
%comparison = arith.cmpi slt, %x, %ten : i32
%c_integer = arith.extui %comparison : i1 to i32
%condition = arith.cmpi ne, %c_integer, %zero : i32
cf.cond_br %condition, ^then, ^else
```

A condition-specific path should branch directly on `%comparison`. In contrast,
`return x < 10;` still needs an integer result. The audit confirms that its
extension remains necessary. Logical NOT and future short-circuit operators
also need the distinction between condition and value contexts.

### 5. Fold constants and reuse proven-pure computations

`repeated_index` emits three identical index extensions and three pointer
offset operations. Canonicalization/CSE reduces each count from three to one.
`repeated_array` emits three array-decay operations and three offsets; each
reduces to one, and constant index extensions disappear.

Folded construction and dominance-safe reuse of constants or operations with
the same immutable SSA operands can avoid this work. This does not permit
deduplicating source expressions with side effects, such as two calls to `f()`
in separate occurrences of `a[f()]`.

## Changes that belong in LLVM lowering

The typed `tensorium.array_decay` operation is meaningful before LLVM lowering.
Its current LLVM expansion emits a `[0, 0]` GEP even though both input and result
are the same opaque pointer type and the address is unchanged. The lowering
can replace this case with the converted base pointer directly while retaining
the typed Tensorium operation for earlier analyses/protection.

`pointer_difference` expands the element stride into a null pointer, a GEP by
one element and a pointer-to-integer conversion before division. All three
operations survive the tested MLIR cleanup. For a statically sized pointee,
the lowering can materialize its allocation stride directly from the configured
target layout. It must not substitute the compiler host's size or the raw bit
width of a type for ABI allocation size/alignment.

## Transformations that still need analysis

`alias_call` passes a local object's address to an unknown function. Its storage
and reload are necessary; the native test makes the callee write a new value.
`alias_store` writes through `q` between two reads through `p`. The audit executes
both `p == q` and disjoint cases. Merging those reads without an alias proof
would change the result.

Interprocedural escape analysis, general load/store forwarding, aggregate
scalar replacement and loop transformations belong in reusable analyses and
passes. Adding the appropriate `PromotableAllocationOpInterface` and
`PromotableMemOpInterface` implementations would also allow Tensorium-level
promotion before lowering away its pointer/object semantics.

## Recommended implementation order

1. Introduce an explicit per-declaration binding choice: SSA value or `Address`.
   Start with initialized, immutable, non-addressed scalar locals and read-only
   parameters. Retain real storage for address-observable objects and aggregates.
2. Emit conditions directly as `i1`; fold constants and trivial pure conversions.
3. Build SSA joins for mutable locals using block arguments and branch operands.
   Snapshot bindings per branch and handle loop-carried values, `break` and
   `continue`. Updating one global value map across both branches is incorrect.
4. Define lexical lifetime/object identity and stack management before broader
   allocation hoisting or VLA support. Preserve source locations while emitting
   the IR; do not rely on memory slots to retain variable/debug identities.
5. Add Tensorium promotion interfaces and effect-aware cleanup for cases left
   in memory; simplify the target-dependent LLVM expansions separately.

## Reproduce the IR comparison

From the repository root, with a configured MLIR build:

```sh
build-mlir/cc1 --emit-mlir docs/audits/memory_codegen_cases.c > /tmp/memory.raw.mlir
build-mlir/tensorium-opt /tmp/memory.raw.mlir \
  --canonicalize --cse -o /tmp/memory.canonical.mlir
build-mlir/tensorium-opt /tmp/memory.raw.mlir \
  --mem2reg -o /tmp/memory.tensorium-mem2reg.mlir
build-mlir/tensorium-opt /tmp/memory.raw.mlir \
  --convert-tensorium-to-llvm -o /tmp/memory.llvm.mlir
build-mlir/tensorium-opt /tmp/memory.raw.mlir \
  --convert-tensorium-to-llvm --sroa --mem2reg --canonicalize --cse \
  -o /tmp/memory.promoted.mlir
```

The audit executed the raw, canonicalized and LLVM-promoted variants as native
code at both `-O0` and `-O2`: six successful runs, including the aliasing cases
and a typed external heap wrapper. This checks the examples' behavior, not
general correctness of a future SSA generator or a performance speedup.
