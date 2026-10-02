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
```

By default, `cc1` parses the supported C subset and exits with status 1 on an
error, or 0 on success. Valid inputs produce no output; code generation is not
implemented yet. `--dump-tokens` runs only the lexer and prints the token stream,
so it does not check syntax.

Diagnostics include the source location, source line and caret. Colors are
automatic on supported terminals and respect `NO_COLOR`; use
`-fcolor-diagnostics` or `-fno-color-diagnostics` to override them. Each invocation
accepts one input file.
