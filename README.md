# Tensorium_cc
A nice frontend for C language with a future LLVM/MLIR Backend
WIP atm

```sh
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build -j 2
ctest --test-dir build --output-on-failure

./build/cc1 Tests/function_body.c
./build/cc1 Tests/invalid_missing_semicolon.c
./build/cc1 --dump-tokens Tests/function_body.c
./build/cc1 --dump-ast Tests/ast_calls.c
./build/cc1 --dump-sema Tests/ast_calls.c
./build/cc1 -E Tests/preprocessor/program.c
./build/cc1 --dump-ast Tests/preprocessor/macros.c
./build/cc1 -I path/to/headers input.c
cmake --build build --target ast_audit
```

By default, `cc1` preprocesses, parses and semantically checks the supported C subset and exits with status 1 on an
error, or 0 on success. Valid inputs produce no output; code generation is not
implemented yet. `--dump-tokens` runs only the lexer and prints the token stream,
so it does not check syntax.

`--dump-ast` prints declarations, function parameters, blocks, returns and
supported expressions after successful parsing. String token spelling stays
separate from decoded bytes; AST literals escape control characters for display.
No AST is printed when parsing fails. This option stops before semantic analysis,
so it can inspect syntactically valid inputs with semantic errors.

`--dump-sema` runs the complete frontend and prints expression types, lvalue/value
categories and inserted implicit conversions. Semantic errors produce source
diagnostics and status 1 without an AST. The default invocation also runs Sema.

Optional MLIR emission is enabled at build time with `-DCC1_ENABLE_MLIR=ON`
and an LLVM/MLIR CMake package (`MLIR_DIR` and, if needed, `LLVM_DIR`).
Run `cc1 --emit-mlir input.c` to request emission after successful Sema.
The current generator emits and verifies empty modules, function prototypes and
definitions returning `int` or `void`. Scalar parameters are mapped to block
arguments using their AST declarations. Automatic scalar locals use `memref`
storage with initializer stores and loads; simple assignment to locals and
parameters is supported. Numeric integer literals, integer `+`, `-`, `*`,
Sema's integer conversions, nested blocks, expression statements and direct
function calls are supported. Compatible function redeclarations share one
symbol, including prototypes followed by definitions. Falling off the end of
`main` returns zero; void functions receive an implicit empty return. Other
non-void functions currently require an explicit return.
Globals, static/extern local storage, volatile objects, indirect or variadic
calls, control flow, floating-point literals/arithmetic/conversions and other
operators still produce explicit unsupported-codegen diagnostics. Without `--emit-mlir`,
the CLI finishes after Sema even when MLIR is enabled in the build.
This option cannot be combined with `-E` or a dump option. Builds without MLIR
reject it with instructions to enable the backend.

The supported subset includes global declarations, function prototypes and
definitions, forward struct/union/enum declarations, nested blocks, expression
statements and returns. Calls, unary/binary/conditional operators, indexing,
member access and `sizeof` are parsed. Blocks support local declarations and
scalar initializers, with one declarator per declaration and scoped typedef
lookup. Global initializers, casts and control-flow statements remain unimplemented;
Sema checks names, redeclarations, signatures, calls, returns, lvalues, arithmetic
and pointer operations for the parsed subset. Code generation remains future
work. See [the Sema/backend contract](docs/sema.md) for coverage, ownership and
current limitations.

The preprocessor supports object-like and function-like `#define` macros,
C99 variadic macros (`...`/`__VA_ARGS__`), stringification (`#`), token pasting
(`##`), and `#undef`, nested
`#if`/`#ifdef`/`#ifndef`/`#elif`/`#else`/`#endif`, `defined`, integer expressions
with short-circuit evaluation, `#error`, and `#pragma once`. Backslash-newline
splicing precedes comment removal. Macro expansion keeps token boundaries and
suppresses direct and indirect self-reference. Arguments are expanded before
substitution unless stringified or pasted; the result is rescanned with the
remaining input. Empty arguments and calls spanning ordinary newlines are
supported. Invalid parameters, argument counts and token pastes are diagnosed.
Adjacent ordinary string literals are decoded separately and joined before parsing.

Predefined macros include `__FILE__`, `__LINE__`, `__DATE__`, `__TIME__`, `__STDC__`,
`__STDC_VERSION__` (according to `LangOptions`, absent in C89), and
`__STDC_HOSTED__` (0: no hosted standard library is provided). The CLI currently
selects C99, so `__STDC_VERSION__` is `199901L`; this identifies the selected
language mode, not full C99 support. File and line macros use the invocation
location, including headers; a function-like macro's body uses the closing
parenthesis line, while prescanned arguments keep their own lines.
Standard predefined macros cannot be redefined
or undefined.

Quoted includes search beside the including file, then the explicit `-I`
directories; angle-bracket includes search the `-I` directories only. Macros
are shared across included files. Tokens and diagnostics retain original file
and physical-line locations, including headers and continued lines.

`-E` emits token-separated text without line markers or C syntax checking.
`--dump-tokens` continues to inspect the raw lexer. This is a first preprocessor
subset: `#line`, command-line macro definitions (`-D`/`-U`), implicit
system-header search paths, GNU variadic extensions and `__VA_OPT__` remain
unimplemented. C99 variadic calls with named parameters must include the comma
before an empty variadic argument. Unsupported active directives
produce a diagnostic. Wide literals and the full C integer-literal rules are
also outside the current frontend's supported subset.

CTest checks AST structure and exact dumps. `ast_audit` writes a report to
`build/ast-audit/report.md`, including deliberately invalid and unsupported
inputs; generating that report does not mean every input was accepted.
When Clang and Python are available, CTest also compares deterministic macro
expansions with Clang's C99 preprocessor, ignoring formatting between tokens.

Diagnostics include the source location, source line and caret. Colors are
automatic on supported terminals and respect `NO_COLOR`; use
`-fcolor-diagnostics` or `-fno-color-diagnostics` to override them. Each invocation
accepts one input file.
