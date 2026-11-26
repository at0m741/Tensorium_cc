# Tensorium MLIR backend architecture

The Tensorium dialect, scalar Tensorium-to-MemRef compatibility conversion,
direct Tensorium-to-LLVM conversion and `tensorium-opt` are implemented.
`cc1_codegen` emits typed object memory, fixed-size array objects, pointer
derivation, nulls, comparisons and differences, alongside `arith`, `cf` and
`func`. MemProtect, dedicated pipeline libraries and the protection runtime
described below remain planned components.

`--emit-mlir` prints raw Tensorium IR with target layout and triple attributes.
`tensorium-opt --convert-tensorium-to-llvm` lowers supported C memory and
`arith/cf/func` directly to the LLVM dialect. The MemRef conversion remains
available for its scalar subset. Native tests exercise both paths and execute
generated programs at `-O0` and `-O2`. See [C memory support](memory.md).

## Current frontend generation policy

The frontend performs a declaration-use analysis before emitting each function.
Read-only, non-addressed scalar parameters and eligible initialized automatic
locals are bound directly to SSA values. Writing through a pointer remains a
memory access but does not require storage for an unchanged pointer variable.
Modified or address-observable objects and fixed-size arrays retain Tensorium
memory operations.

All fixed-size allocations created by CC1 are inserted in the function entry
block. Local initializers and their stores remain at the declaration's CFG
position, including in nested loops and after `continue`/`break` conditions.
The conversion itself preserves the placement of allocations in its input;
hoisting is a frontend generation policy, not a Tensorium-to-LLVM optimization.
Explicit lexical lifetime operations and VLA stack management remain planned.

The existing raw-IR and native regression cases are documented in
[the SSA test guide](../Tests/CodeGen/SSA/README.md). General SSA joins for mutable
variables, direct `i1` condition generation and Tensorium promotion interfaces
remain future work.

File-scope objects use `tensorium.global` and `tensorium.get_global`, declared
in `TensoriumGlobalOps.td`. `TensoriumEnums.td` generates typed pointer predicates
and global linkage attributes. Integer and zero initializers lower to LLVM
globals; symbol references lower to LLVM addresses. `TensoriumAttrs.td` defines
`GlobalAddressAttr` for static pointer initializers, lowered to LLVM address
initializer regions without runtime code. See [global object support](globals.md).

## Module boundaries

The C frontend stays under `include/cc1` and its existing `lib` directories.
`lib/CodeGen` translates the checked C AST into MLIR. Reusable MLIR components
have public headers under `include/tensorium` and implementations under
`lib/tensorium`; their includes use the `tensorium/` prefix.

Tensorium MLIR libraries must not depend on the C AST, parser, Sema or diagnostic
engine. Types, object identity, lifetime information, source locations and target
layout needed after generation must be represented in the IR. `Address` remains
a frontend generation helper, not a dialect type or a runtime ABI structure.

```text
include/tensorium/
  Dialect/
    Tensorium/
      IR/                 # Dialect, type and operation declarations; TableGen
      Transforms/         # Tensorium pass declarations
    MemProtect/
      IR/                 # Protected pointers and memory operation declarations
      Transforms/         # MemProtect pass declarations
  Conversion/             # Public conversion pass and pattern declarations
  Pipelines/              # Pipeline options and registration
  Runtime/MemProtect.h    # C ABI; no dependency on MLIR headers

lib/tensorium/
  Dialect/
    Tensorium/
      IR/                 # Registration, types, verifiers and operation semantics
      Transforms/         # Analyses, canonicalization and optimization passes
    MemProtect/
      IR/
      Transforms/
  Conversion/
    TensoriumToMemRef/    # Initial compatibility lowering for scalar storage
    TensoriumToMemProtect/
    TensoriumToLLVM/
    MemProtectToLLVM/
  Pipelines/              # Compose and register passes

runtime/MemProtect/       # Allocation metadata, validation and failure handlers
tools/tensorium-opt/      # MLIR optimizer with project dialects and passes
Tests/
  Dialect/Tensorium/
  Dialect/MemProtect/
  Conversion/
  Pipelines/
  Runtime/MemProtect/
```

Create these components when implementing them; this tree does not require empty
libraries or placeholder APIs now. Each implemented component gets its own
`CMakeLists.txt`. Generated TableGen files belong in the mirrored build include
directory, never in the source tree.

## Responsibilities

| Component | Owns | Dependency direction |
| --- | --- | --- |
| `cc1_codegen` | AST-to-IR generation and backend invocation adapter | C AST, Tensorium IR, pipeline APIs |
| Tensorium IR | Object, pointer and memory semantics independent of protection policy | MLIR interfaces and required standard dialects |
| Tensorium transforms | Analyses and optimizations of Tensorium operations | Tensorium IR |
| MemProtect IR | Protected pointer semantics, checked accesses and allocation validity | MLIR interfaces |
| MemProtect transforms | Analyses and transformations of protected operations | MemProtect IR |
| Conversion libraries | Type conversion, operation rewriting and legality rules | Source and destination dialects |
| Pipelines | Pass order, options and registration | Transform and conversion libraries |
| MemProtect runtime | Executable protection support through a defined C ABI | Platform facilities; no frontend or MLIR dependency |

Use separate CMake targets: `tensorium_ir`, `tensorium_to_llvm` and `tensorium_to_memref` currently
exist; add `memprotect_ir`, transform and further conversion targets as their
implementations arrive. A future `tensorium_pipelines` composes these targets;
`tensorium-opt` links the registered dialects, passes and MLIR optimizer support.
The frontend and core tests must continue to build with `CC1_ENABLE_MLIR=OFF`.
The runtime gets an independent build option and target.

## IR and conversion contracts

The `tensorium` dialect has typed pointers, for example `!tensorium.ptr<i32>`,
and fixed-size array objects such as `!tensorium.array<4, i32>`, together with
local allocation, load, store, pointer offsets, array decay, null pointers,
pointer comparisons and element differences. Arithmetic, branches and functions
use `arith`, `cf` and `func`. General pointer casts and explicit lifetime
operations remain future work.

Specify allocation extent, alignment, object identity, pointer derivation and
access width before defining operations. Record the target layout in the module;
conversion passes must not infer it from the compiler host. Static source type
and runtime allocation type are separate concepts.

`--convert-tensorium-to-llvm` requires an explicit valid `llvm.data_layout`, even
for an input containing only standard operations. CC1 emits this attribute and
`llvm.target_triple`; hand-written or older saved modules must provide their
intended target layout. Canonicalization does not supply a missing target.

The MemProtect dialect uses the `memprotect` namespace and an abstract protected
pointer type. Define bounds propagation, access checks, lifetime validation and
failure behavior before selecting its concrete LLVM representation. Bounds alone
do not establish that an allocation remains alive. Allocation identity and
generation, for example, can reference runtime lifetime metadata.

IR implementations own verifiers and accurate memory-effect/speculation
interfaces. Optimizations must preserve protection semantics, including the
defined observability of failed checks. Checks may be removed only when the
analysis proves the access safe under those semantics.

Conversion passes own pointer representation changes. Converting a pointer type
also requires converting function signatures, calls, returns, block arguments
and pointer-valued storage. External calls require an explicit ABI boundary;
metadata must not disappear implicitly at a call or cast. Dialect libraries must
not depend on their lowering libraries.

## Pipeline ownership

The intended protected pipeline is:

```text
checked C AST
  -> Tensorium memory operations + arith/cf/func
  -> semantics-preserving Tensorium optimizations
  -> Tensorium-to-MemProtect conversion
  -> MemProtect optimizations
  -> MemProtect and standard dialects to LLVM
  -> native code linked with the MemProtect runtime
```

`MLIRGen` only generates IR. A separate backend adapter in `lib/CodeGen` invokes
the selected pipeline. `CompilerInstance` coordinates frontend execution and
output actions; it does not implement dialect transformations.

The Tensorium-to-MemRef conversion preserves the original scalar storage
behavior. It is a compatibility stage, not a specification for general C
pointers or memory protection. The implemented Tensorium-to-LLVM conversion
provides the explicitly selected unprotected path for stored pointers, arrays
and ordinary external pointer ABIs.

The implemented compatibility conversion maps pointers to integer/float objects
to rank-zero memrefs. It converts internal function signatures, direct calls,
returns and `cf` block arguments together with memory operations. Nested
pointers (including pointer-valued storage), external declarations carrying
Tensorium pointers and unsupported pointer consumers fail conversion. There is
no implicit pointer ABI adapter or metadata-dropping cast. Scalar external
declarations remain supported. This conversion does not add runtime protection.

`tensorium-opt` provides textual IR input and independent pass execution.
Tests cover dialect parsing and verification, conversion legality and execution.
The stock `mlir-opt` can consume output only after project operations have been
lowered to the standard dialects it registers.

## MOpt analyses

The optional MOpt libraries live under `include/mopt` and `lib/mopt` and are
independent of the frontend and Tensorium dialect. `tensorium-opt` registers
their `mopt-analyze-loops` pass when `CC1_ENABLE_MOPT` is enabled. Analysis and
planning return data; the reporting pass writes to stderr and preserves IR.

These analyses currently consume `scf.for` and indexed MemRef accesses, whereas
CC1 emits CFG loops. They are available for structured MLIR inputs but are not
automatically applied to C output. See [MOpt integration](mopt.md) for the target
contract, tests and the remaining structured-loop and memory integration work.

## Completed first implementation milestone

1. Tensorium IR definitions, registration and the `tensorium_ir` target.
2. Typed local allocation, load and store with verifier/effect tests.
3. `tensorium_to_memref` for the current scalar subset, including conversion
   of pointer-carrying signatures and block arguments within that subset.
4. `tensorium-opt` registers the dialect, compatibility pass and standard
   canonicalization/CSE passes.
5. CodeGen memory helpers emit Tensorium operations; control-flow generation
   is preserved. Tests distinguish raw generated IR from lowered output.
6. Existing tests and native `for` behavior cover the new compatibility pipeline.
   Builds with MLIR enabled and disabled must continue to pass.

Fixed-size arrays and pointer arithmetic are implemented by the subsequent C
memory milestone described in [memory.md](memory.md). Define and implement
MemProtect incrementally once the required base memory semantics are tested.

Subsequent frontend changes add read-only parameter/local SSA bindings and
entry-block storage for fixed-size automatic objects, with initialization left
at the source declaration. The earlier [generation audit](audits/memory_codegen_audit.md)
is retained as a historical baseline; its measured operation counts predate
these changes.
