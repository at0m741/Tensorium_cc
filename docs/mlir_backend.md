# Tensorium MLIR backend architecture

This document defines the planned backend structure. The dialect libraries,
conversions, optimizer tool and MemProtect runtime described below are not yet
implemented. Today, `cc1_codegen` emits `arith`, `cf`, `func` and scalar `memref`
operations directly.

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

Use separate CMake targets, initially `tensorium_ir` and
`tensorium_to_memref`, then `memprotect_ir`, transform and conversion targets
as their implementations arrive. `tensorium_pipelines` composes these targets;
`tensorium-opt` links the registered dialects, passes and MLIR optimizer support.
The frontend and core tests must continue to build with `CC1_ENABLE_MLIR=OFF`.
The runtime gets an independent build option and target.

## IR and conversion contracts

Start with the `tensorium` dialect namespace and a typed pointer, for example
`!tensorium.ptr<i32>`, together with local allocation, load and store operations.
Retain the existing `arith`, `cf` and `func` operations for arithmetic, branches
and functions. Add pointer offsets, casts and explicit lifetime operations as
their semantics and frontend support are implemented.

Specify allocation extent, alignment, object identity, pointer derivation and
access width before defining operations. Record the target layout in the module;
conversion passes must not infer it from the compiler host. Static source type
and runtime allocation type are separate concepts.

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

The first Tensorium-to-MemRef conversion preserves the existing scalar storage
behavior and native test pipeline. It is a migration stage, not a specification
for general C pointers or memory protection. A later direct Tensorium-to-LLVM
conversion can provide an explicitly selected unprotected pipeline.

`tensorium-opt` provides textual IR input and independent pass execution.
Tests cover dialect parsing and verification, conversion legality and execution.
The stock `mlir-opt` can consume output only after project operations have been
lowered to the standard dialects it registers.

## First implementation milestone

1. Add Tensorium IR definitions, registration and the `tensorium_ir` target.
2. Implement typed local allocation, load and store with verifier/effect tests.
3. Add `tensorium_to_memref` for the current scalar subset, including conversion
   of pointer-carrying signatures and block arguments within that subset.
4. Add `tensorium-opt` and register the dialect and conversion pass.
5. Change the CodeGen memory helpers to emit Tensorium operations; preserve
   control-flow generation. Make the compatibility pipeline explicit in tests
   and distinguish raw generated IR from lowered output.
6. Run the existing tests and compare native behavior, including the `for`
   regression suite. Validate builds with MLIR enabled and disabled.

Extend to arrays and pointer arithmetic after this milestone. Define and
implement MemProtect incrementally once the base memory semantics are tested.
