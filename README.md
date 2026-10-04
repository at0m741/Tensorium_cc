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
./build/cc1 -E Tests/preprocessor/program.c
./build/cc1 -I path/to/headers input.c
cmake --build build --target ast_audit
```

By default, `cc1` preprocesses and parses the supported C subset and exits with status 1 on an
error, or 0 on success. Valid inputs produce no output; code generation is not
implemented yet. `--dump-tokens` runs only the lexer and prints the token stream,
so it does not check syntax.

`--dump-ast` prints declarations, function parameters, blocks, returns and
supported expressions after successful parsing. String token spelling stays
separate from decoded bytes; AST literals escape control characters for display.
No AST is printed when parsing fails.

The supported subset includes global declarations, function prototypes and
definitions, forward struct/union/enum declarations, nested blocks, expression
statements and returns. Calls, unary/binary/conditional operators, indexing,
member access and `sizeof` are parsed. Local declarations and initializers,
casts and control-flow statements are still unimplemented;
semantic analysis and code generation remain future work.

The preprocessor supports object-like `#define` macros and `#undef`, nested
`#if`/`#ifdef`/`#ifndef`/`#elif`/`#else`/`#endif`, `defined`, integer expressions
with short-circuit evaluation, `#error`, and `#pragma once`. Backslash-newline
splicing precedes comment removal. Macro expansion keeps token boundaries and
suppresses direct and indirect self-reference.

Quoted includes search beside the including file, then the explicit `-I`
directories; angle-bracket includes search the `-I` directories only. Macros
are shared across included files. Tokens and diagnostics retain original file
and physical-line locations, including headers and continued lines.

`-E` emits token-separated text without line markers or C syntax checking.
`--dump-tokens` continues to inspect the raw lexer. This is a first preprocessor
subset: function-like macros, `#`/`##`, `#line`, predefined macros and implicit
system-header search paths remain unimplemented. Unsupported active directives
produce a diagnostic. Wide literals and the full C integer-literal rules are
also outside the current frontend's supported subset.

CTest checks AST structure and exact dumps. `ast_audit` writes a report to
`build/ast-audit/report.md`, including deliberately invalid and unsupported
inputs; generating that report does not mean every input was accepted.

Diagnostics include the source location, source line and caret. Colors are
automatic on supported terminals and respect `NO_COLOR`; use
`-fcolor-diagnostics` or `-fno-color-diagnostics` to override them. Each invocation
accepts one input file.
