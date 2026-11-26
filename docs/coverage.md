# C99 implementation coverage and remaining work

Last reviewed: 2026-11-26.

This inventory describes the current source and the regression cases available
in the repository. It is not a C99 conformance report or a record that every
test passes on the current checkout. No compiler tests were run to prepare this
document. The older measured IR audit remains a
[separate historical baseline](audits/memory_codegen_audit.md).

The language target is C99 on a selected platform and ABI. `LangOptions` defaults
to its C99 configuration, but flags and predefined macros do not establish that
every corresponding language feature is implemented. A C construct accepted by
the parser and Sema may still be rejected by CodeGen.

## Status terminology

- **Supported:** implemented for the subset described in the row, with relevant
  regression cases available; this does not imply support for every C99 variant.
- **Partial:** only some cases or compiler stages are implemented.
- **In progress:** recently added frontend work without a complete native path.
- **Missing:** the normal C compilation path does not implement the feature.

## What already works

The implemented path is:

```text
C source -> lexer/preprocessor -> parser -> typed AST
         -> Tensorium memory + arith/cf/func
         -> LLVM dialect -> LLVM IR -> native compilation and execution
```

CC1 performs the frontend and optional raw MLIR generation. `tensorium-opt`
performs project conversions. In MLIR builds, `cc1` uses the shared native
driver to orchestrate those tools, LLVM IR translation and Clang compilation/
linking across translation units. Explicit frontend actions bypass native
compilation; `tensoriumcc` is an alternate driver command. See
[the native driver guide](driver.md).

Useful supported inputs include functions using integer calculations, `if`,
`while`, `for`, local variables, fixed-size arrays and typed pointer operations.
Native fixtures cover pointer aliases, nested pointers, external pointer calls,
SSA bindings, loop storage and a subset of global objects and static address
initializers. Strings and many ordinary C operators still block otherwise small
programs.

## Types, literals and expressions

| Feature | Frontend | MLIR generation and remaining limits |
| --- | --- | --- |
| Integer types and promotions | Supported subset | Integer type lowering and sign/zero extension/truncation exist. Function result types remain restricted separately. |
| Integer literals and suffixes | Partial | Supported within the lexer's signed `long long` value range. The full unsigned literal range is not supported. |
| `float`, `double`, `long double` | Partial | Float/double object storage and type lowering exist. Floating literals, arithmetic, comparisons and conversions are not generally generated; `long double` lowering is missing. |
| `_Bool` | Partial internal support | A BOOL type and integer-to-BOOL conversion exist internally. Source `_Bool` syntax is not implemented. |
| Complex types | Missing | No complete C99 complex type, expression or call support. |
| Character and string literals | Basic literals parsed and typed | Character literal emission and string object storage are missing. Wide literals and adjacent string concatenation need additional frontend work. |
| Integer `+`, `-`, `*` | Supported | Generated with integer promotions/conversions from Sema. |
| Integer `/`, `%`, bitwise operators and shifts | Parsed and checked | Missing CodeGen, including unary `~`. |
| Simple assignment | Supported subset | Uses a typed lvalue address and stores a converted value. |
| Integer `+=`, `-=`, `*=` | Supported | Evaluates the left address once, calculates in the promoted type and converts back. Other integer compound operators are missing in CodeGen. |
| Integer/pointer `++`, `--` | Supported | Prefix and postfix result timing are distinguished. Floating updates are not generated. |
| Integer and pointer comparisons | Supported subset | Includes signed/unsigned integer predicates and supported typed pointer comparisons. General floating comparisons are missing. |
| Unary `+`, `-`, `!` | Partial | Integer unary arithmetic and integer/pointer logical not are generated. General floating cases are missing. |
| `&&`, `\|\|`, `?:`, comma expressions | Parsed and checked | Missing CodeGen. Logical and conditional operators require lazy evaluation of their branches. |
| Implicit conversions | Partial | Integral, lvalue-read, array-decay and supported pointer/null conversions exist. Floating and default-argument-promotion paths are incomplete in CodeGen. |
| Explicit casts | Missing syntax | `CastExpr` exists in the AST, but the parser does not construct general explicit cast expressions. |
| `sizeof` | Supported subset | Target-sized results for supported complete scalar/pointer/fixed-array types. Operands are not evaluated. Record, VLA and unsupported numeric type cases remain open. |
| General constant expressions | Missing complete evaluator | A shared AST evaluator handles typed integer literals, arithmetic conversions and unary/binary `+`, `-`, `*` for globals. Other constant expressions and computed bounds remain unfinished. |

Primary sources:
[expression generation](../lib/CodeGen/MLIRGenExpr.cpp),
[binary generation](../lib/CodeGen/MLIRGenBinary.cpp),
[type conversion](../lib/CodeGen/MLIRGenTypes.cpp) and
[semantic analysis](sema.md).

## Declarations, control flow and functions

| Feature | Frontend | MLIR generation and remaining limits |
| --- | --- | --- |
| Automatic declarations and scalar initializers | Supported subset | Includes nested blocks, declaration identity and shadowing. Multiple declarators such as `int a, b;` are not parsed. |
| `typedef` and basic qualifiers | Supported subset | Typedef resolution and `const` checks exist. `volatile` memory generation is rejected; `restrict` syntax/contracts remain missing. |
| Function prototypes, definitions and recursion | Supported subset | Direct calls and function symbol canonicalization exist. See the known prototype-binding issue below. |
| Function results | Supported subset | CodeGen accepts `int`/`unsigned int`, `long`/`unsigned long`, `long long`/`unsigned long long`, `void` and supported pointer results. Narrow integer result ABI attributes and floating results remain missing. |
| Function pointer declarations and indirect calls | Parsed and checked | Function-pointer types and indirect calls are not generated. |
| Variadic functions | Parsed and checked | CodeGen rejects variadic declarations. ABI-specific `va_list` and variadic access support are missing. |
| `inline` and old-style definitions | Missing complete support | C99 inline/linkage rules and old-style function definitions need implementation. |
| `return`, `if`/`else`, `while`, `for`, `break`, `continue` | Supported subset | Integer and supported pointer conditions are emitted as CFG control flow. Floating conditions are not generated. |
| `do`/`while`, `switch`/`case`, labels and `goto` | Missing syntax | AST node placeholders do not provide a parser, Sema or CodeGen implementation. |
| File-scope declarations and initializers | Supported subset | Compatible declarations merge into one symbol; integer constants, null pointers, scalar/array addresses, constant pointer offsets and zero-initialized objects lower to LLVM. Aggregate initializers remain missing. |
| Static local objects | Partial frontend | Persistent storage, initialization and CodeGen remain missing. |
| Struct/union/enum definitions | Missing syntax | Forward tags exist, but definitions, enumerators and useful member access are unavailable. |
| Record layout and aggregate ABI | Missing | Field offsets, bit-fields, flexible array members, aggregate copies and native argument/result classification need implementation. |
| Aggregate and designated initializers | Missing | Brace initializers, array string initializers and C99 designated initialization are not implemented. |
| Compound literals | Missing | C99 expressions such as `(struct Point){1, 2}` need syntax, type checking and storage semantics. |

Primary sources:
[declaration parsing](../lib/Parse/ParseDecl.cpp),
[statement parsing](../lib/Parse/ParseStmt.cpp),
[statement generation](../lib/CodeGen/MLIRGenStmt.cpp) and
[function generation](../lib/CodeGen/MLIRGenFunction.cpp).

## Object memory and SSA

| Feature | Status | Current behavior or remaining work |
| --- | --- | --- |
| Typed local allocation, load and store | Supported | Tensorium represents object types before native lowering. |
| Pointer variables and nested pointers | Supported subset | Includes address-taking, rebinding and pointer-valued storage. |
| Fixed-size arrays and indexing | Supported subset | Includes multidimensional arrays, arrays of pointers and pointers to array rows. Bounds must be positive integer literals after preprocessing. |
| Pointer arithmetic and differences | Supported subset | Includes integer offsets, pointer `+=`/`-=`, element differences, nulls and supported comparisons. Normal C validity requirements still apply. |
| `void *` and general pointer casts | Supported subset | Object pointers convert to/from void pointers using a typed dialect cast, including storage, globals and native calls. Void dereferences/arithmetic, qualifier drops and general explicit casts remain unsupported. |
| Read-only parameter SSA bindings | Supported | Parameters that are neither written nor addressed remain entry arguments across CFG control flow. |
| Initialized local SSA bindings | Supported subset | Non-volatile, unmodified, non-addressed scalar locals can use initializer values directly. Evaluated self-references retain a memory fallback. |
| Mutable, addressed or uninitialized locals | Supported subset | Retain memory storage. An uninitialized local receives no implicit store. |
| Fixed-size allocation placement | Supported | CC1 inserts storage in function entry and leaves initialization at the source declaration, including in nested loops. |
| General SSA joins for mutable variables | Missing | Branch results and loop-carried values need block arguments and branch operands. Current memory fallbacks provide the implemented path. |
| Global/static object generation | Partial | TableGen global/address operations, symbol/type verification and LLVM lowering support file-scope objects with internal/external linkage. Static local objects remain missing. |
| Variable-length arrays | Missing | Runtime extents, dynamic allocation and appropriate scope restoration need implementation. |
| Explicit lexical lifetime operations | Missing | Entry-block storage placement does not add source lifetime boundaries or protection metadata. |
| `volatile` and `restrict` semantics | Missing complete support | Volatile memory accesses are rejected by CodeGen; restrict contracts are absent. |
| Ordinary heap APIs | Partial native-call support | The void-pointer ABI barrier is removed; correctly declared external scalar/pointer APIs can use native linkage. SDK headers and broader allocation/library integration still need coverage. |
| Protected pointers and checked accesses | Planned | MemProtect, bounds/lifetime checks and the protection runtime are not implemented. |

See [the memory contract](memory.md) and
[the SSA and loop storage test guide](../Tests/CodeGen/SSA/README.md).

## Preprocessing, tools and target integration

| Area | Status | Current behavior or remaining work |
| --- | --- | --- |
| Preprocessing | Supported subset | Includes, `-I`, object/function/variadic macros, stringification/token pasting, conditional directives, `#error`, `#pragma once` and selected predefined macros exist. Full translation-phase/directive conformance remains unfinished. |
| CLI actions | Supported subset | MLIR-enabled `cc1` defaults to native compilation/linking; `-fsyntax-only`, `-E`, dumps and `--emit-mlir` select a single-TU frontend action. Without MLIR, default checking remains. |
| Driver output modes | Supported subset | `cc1` (also `tensoriumcc`) provides `-c`, `-S`, `-o`, multi-source native linking, object/archive inputs and `-L`/`-l`. |
| Driver configuration | Partial | Driver include/library paths, native optimization, verbosity and tool overrides exist. CLI `-D`, `-U`, `-std`, sysroot and target selection remain missing. |
| Target information | Partial | i386, x86-64 and AArch64 presets and explicit LLVM layout handling exist. CC1 normally selects the host preset; these are not a complete cross-compilation toolchain. |
| Tensorium-to-LLVM conversion | Supported subset | Converts supported memory, arithmetic, CFG and function operations; explicit valid `llvm.data_layout` is required. |
| Tensorium-to-MemRef conversion | Compatibility subset | Scalar pointer/storage path with internal signatures and block arguments; general pointer-valued storage and external pointer ABIs are rejected. |
| MOpt | Analysis/planning integration | Consumes structured `scf.for` and indexed MemRef inputs. CC1 emits CFG loops; automatic reconstruction, vectorization, tiling and unrolling are not implemented. |
| Diagnostics and debugging | Partial | Frontend diagnostics retain source locations. Most emitted operations use unknown locations; native debug information and comprehensive warnings/flow analysis remain missing. |
| Standard library and SDK integration | Partial | Typed external calls are exercised with Clang helpers. Normal system-header compatibility, complete native ABI coverage and hosted-library integration remain unfinished. |

LLVM and platform tools can provide machine-code generation, assembly, linking
and library implementations. Those components do not all need to be rewritten;
the project still needs to integrate them and emit a compatible ABI. Supporting
a feature internally does not by itself make arbitrary platform headers usable.

## Current global-object work and known issues

File-scope objects now use TableGen-defined global/address operations and typed
linkage attributes. CodeGen merges canonical declarations, emits integer or zero
initializers, and resolves accesses from functions. The shared AST evaluator
checks target-width integer arithmetic and rejects signed overflow in its
supported subset. The AST address evaluator supports direct object addresses,
array decay, constant pointer offsets and nested indexing, with bounds checks.
A generated symbolic attribute carries the typed path into LLVM initializer
regions. The function-prototype symbol-binding issue has been fixed.

Remaining work includes complete C99 constant-expression legality in Sema,
`sizeof` and other constant operators, aggregate/string initializers, static
locals and broader tentative-definition/ABI
coverage. [Global fixtures](../Tests/CodeGen/Globals/) now register emission,
structure, rejection and native execution tests; their latest execution status
must be checked separately. See [global object support](globals.md).

More generally, Sema has no complete missing-return or unreachable-code
analysis. CodeGen diagnoses some non-void fallthrough cases during emission.
Language-mode flags and AST/type placeholders must not be used as evidence that
their corresponding source features are supported.

## Validation coverage

Available suites include:

- [Parser tests](../Tests/Parse/ParserTests.cpp) and
  [Sema tests](../Tests/Sema/SemaTests.cpp).
- [Preprocessor tests](../Tests/Lex/PreprocessorTests.cpp), including optional
  differential macro-expansion checks against Clang.
- [Raw MLIR generation tests](../Tests/CodeGen/MLIRGenTests.cpp) and
  [storage analysis tests](../Tests/CodeGen/LocalStorageAnalysisTests.cpp).
- [Real C memory programs](../Tests/CodeGen/Memory/README.md) and
  [SSA/loop storage programs](../Tests/CodeGen/SSA/README.md), with native runtime
  registrations at both `-O0` and `-O2` when the tools are available.
- [Tensorium dialect tests](../Tests/Dialect/Tensorium) and
  [LLVM conversion fixtures](../Tests/Conversion/LLVM), including layout and
  conversion failure cases.
- [MOpt API/integration and optional LIT regressions](mopt.md#tests).

The number of passing CTest entries is not a percentage of C99 coverage. Full
conformance needs broader language cases, interactions between features,
differential execution checks and real multi-file projects. Build/test results
must be recorded separately from this source-based inventory.

## Suggested implementation priorities

For the first practical multi-file C project:

1. Extend global-object constant checking and aggregate initializers,
   including canonical symbols, initialization, reads/writes and linkage cases.
2. Implement missing integer operators and lazy logical/conditional evaluation.
3. Generate character/string literals and common array initialization forms.
4. Extend the native driver with SDK/system include discovery and common build flags.
5. Implement basic records, layout, member access and initialization.
6. Extend pointer conversions, function signatures and library-call compatibility.

Track completion using programs with several `.c`/`.h` files, global state,
strings, structures and fixed-signature libc calls. Add dynamic allocation and
variadic calls as the pointer and ABI support becomes available.

The remaining full-C99 work also includes exact constant/literal semantics,
complete declarations and storage/linkage rules, `_Bool`, `inline`, `restrict`,
volatile access, all control-flow forms, VLA/aggregate semantics, wide characters,
complete floating-point/complex support and the corresponding ABI/header/runtime
integration. C11-only features such as atomics, `_Generic` and thread-local
storage, and platform-specific GNU/Clang extensions, are additional goals rather
than requirements of this C99 baseline.

Direct `i1` condition generation, broader SSA construction, Tensorium promotion
interfaces and advanced transformations are separate optimization work.
Memory-protection passes are a separate extension of the base compiler.
