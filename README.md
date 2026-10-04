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
cmake --build build --target ast_audit
```

By default, `cc1` parses the supported C subset and exits with status 1 on an
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
casts, control-flow statements and preprocessing are still unimplemented;
semantic analysis and code generation remain future work.

CTest checks AST structure and exact dumps. `ast_audit` writes a report to
`build/ast-audit/report.md`, including deliberately invalid and unsupported
inputs; generating that report does not mean every input was accepted.

Diagnostics include the source location, source line and caret. Colors are
automatic on supported terminals and respect `NO_COLOR`; use
`-fcolor-diagnostics` or `-fno-color-diagnostics` to override them. Each invocation
accepts one input file.
