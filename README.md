# Tensorium_cc

A C frontend with an optional MLIR backend. The supported C subset is still
growing.

```sh
cmake -S . -B build -DBUILD_TESTING=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j 2
ctest --test-dir build --output-on-failure
```

Enable MLIR using the configuration directories of the installed LLVM/MLIR:

The backend supports MLIR 20 and 21 using `OpBuilder::create`; MLIR 22 and newer
use the operations' static `create` methods. The installed LLVM version selects
the API at compile time, including the integer constant argument order that
changed in MLIR 21.

```sh
cmake -S . -B build-mlir -DBUILD_TESTING=ON -DCMAKE_BUILD_TYPE=Debug \
  -DCC1_ENABLE_MLIR=ON \
  -DMLIR_DIR=/opt/llvm/lib/cmake/mlir -DLLVM_DIR=/opt/llvm/lib/cmake/llvm
cmake --build build-mlir -j 2
ctest --test-dir build-mlir --output-on-failure
build-mlir/cc1 --emit-mlir Tests/function_body.c
```

`--emit-mlir` emits Tensorium memory operations alongside `arith`, `cf` and
`func`. Lower the scalar storage to standard `memref` operations before using
the stock LLVM/MLIR tools:

```sh
build-mlir/cc1 --emit-mlir Tests/For_stmt.c > /tmp/program.tensorium.mlir
build-mlir/tensorium-opt /tmp/program.tensorium.mlir \
  --convert-tensorium-to-memref -o /tmp/program.memref.mlir
```

The compatibility conversion also handles scalar pointers in internal function
signatures, calls, returns and CFG block arguments. Pointer-valued storage and
external pointer ABIs require a later lowering and are rejected explicitly.

Public headers live in `include/cc1`, implementations in `lib`, and the CLI in
`tools/cc1`. Tests are grouped by component under `Tests`; existing C fixtures
remain available at their original paths. See [the architecture](docs/architecture.md)
for module dependencies and AST ownership, and [semantic analysis](docs/sema.md)
for the frontend/backend contract. The [MLIR backend design](docs/mlir_backend.md)
describes the Tensorium backend and the planned MemProtect protection pipeline.

C++ formatting uses the repository's `.clang-format`. Optimization follows
`CMAKE_BUILD_TYPE`; warning settings are controlled by `CC1_ENABLE_WARNINGS`.
