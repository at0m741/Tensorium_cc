# Tensorium_cc

A C frontend with an optional MLIR backend. The supported C subset is still
growing.

```sh
cmake -S . -B build -DBUILD_TESTING=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j 2
ctest --test-dir build --output-on-failure
```

Enable MLIR using the configuration directories of the installed LLVM/MLIR:

```sh
cmake -S . -B build-mlir -DBUILD_TESTING=ON -DCMAKE_BUILD_TYPE=Debug \
  -DCC1_ENABLE_MLIR=ON \
  -DMLIR_DIR=/opt/llvm/lib/cmake/mlir -DLLVM_DIR=/opt/llvm/lib/cmake/llvm
cmake --build build-mlir -j 2
ctest --test-dir build-mlir --output-on-failure
build-mlir/cc1 --emit-mlir Tests/function_body.c
```

Public headers live in `include/cc1`, implementations in `lib`, and the CLI in
`tools/cc1`. Tests are grouped by component under `Tests`; existing C fixtures
remain available at their original paths. See [the architecture](docs/architecture.md)
for module dependencies and AST ownership, and [semantic analysis](docs/sema.md)
for the frontend/backend contract.

C++ formatting uses the repository's `.clang-format`. Optimization follows
`CMAKE_BUILD_TYPE`; warning settings are controlled by `CC1_ENABLE_WARNINGS`.
