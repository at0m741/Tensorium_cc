# Tensorium_cc

A C frontend with an optional MLIR backend. The supported C subset is still
growing.

See [C99 coverage and remaining work](docs/coverage.md) for the current feature
inventory, the distinction between frontend and CodeGen support, known issues
and implementation priorities.

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

In MLIR builds, `cc1` uses the native compiler driver by default. It accepts `-c`, `-S`,
`-o`, include paths, multiple C sources, objects and static libraries:

```sh
build-mlir/cc1 -I Tests/Driver/fixtures/include \
  Tests/Driver/fixtures/main.c Tests/Driver/fixtures/helper.c \
  -o /tmp/tensorium-program
/tmp/tensorium-program
```

See [the native driver guide](docs/driver.md) for tool discovery, supported flags
and current limitations. Use `cc1 -fsyntax-only` for semantic checking; explicit
dumps, `-E` and `--emit-mlir` remain frontend actions. `tensoriumcc` is an
alternate native driver executable.

A first terminal-output example uses native `write` with a byte buffer:

```sh
build-mlir/cc1 Tests/Driver/fixtures/write.c -o /tmp/write-example
/tmp/write-example
```

This example uses a manual LP64 POSIX declaration; SDK headers and string-literal
emission remain unfinished. See [the write example](docs/driver.md#native-write-example).

`--emit-mlir` emits Tensorium memory operations alongside `arith`, `cf` and
`func`. For C object memory, including stored pointers and fixed-size arrays,
lower directly to LLVM using the target layout recorded by CC1:

```sh
build-mlir/cc1 --emit-mlir Tests/CodeGen/Memory/valid/pointer_arithmetic.c \
  > /tmp/program.tensorium.mlir
build-mlir/tensorium-opt /tmp/program.tensorium.mlir \
  --convert-tensorium-to-llvm -o /tmp/program.llvm.mlir
```

The scalar compatibility conversion to standard `memref` operations remains
available for the original subset:

```sh
build-mlir/cc1 --emit-mlir Tests/For_stmt.c > /tmp/program.tensorium.mlir
build-mlir/tensorium-opt /tmp/program.tensorium.mlir \
  --convert-tensorium-to-memref -o /tmp/program.memref.mlir
```

The compatibility conversion also handles scalar pointers in internal function
signatures, calls, returns and CFG block arguments. It rejects pointer-valued
storage and external pointer ABIs; the direct LLVM conversion supports these
with ordinary native pointers. See [C memory support](docs/memory.md) for the
supported subset, semantics and execution tests.

CodeGen now selects SSA values or memory storage per declaration before emitting
a function. Read-only, non-addressed scalar parameters and eligible initialized
locals stay in SSA across branches and loops. Objects that need storage use
fixed-size allocations in the function entry block; local initializers still
execute at the source declaration, including on each loop iteration. See the
[SSA and loop storage tests](Tests/CodeGen/SSA/README.md).

File-scope objects now use TableGen-defined `tensorium.global` and
`tensorium.get_global` operations, with LLVM lowering for declarations,
internal/external linkage, integer constant initializers, direct object address
initializers (`int *p = &global`, `values + 2`, `&matrix[1][2]`) and zero-initialized
objects. Constant evaluation currently supports integer literals, arithmetic
conversions and unary/binary `+`, `-`, `*`. See [global object support](docs/globals.md)
for limitations and test commands.

For hand-written MLIR, supply an explicit `llvm.data_layout` matching the target
before running `--convert-tensorium-to-llvm`. CC1-generated modules already carry
this attribute and `llvm.target_triple`. Use tools from the same configured build
when multiple LLVM installations coexist.

MLIR builds also enable the optional MOpt analysis libraries and tool, imported
from MLIRArchOpt. They analyze structured `scf.for` loops and indexed `memref`
accesses and produce target-aware vectorization plans; they do not transform IR.

```sh
build-mlir/mopt --dump-target-info Tests/MOpt/01_linear.mlir
build-mlir/tensorium-opt Tests/MOpt/01_linear.mlir \
  --mopt-analyze-loops='mcpu=generic' -o /tmp/analyzed.mlir
```

`tensorium-opt` writes MOpt reports to stderr and keeps stdout for MLIR. Disable
this component with `-DCC1_ENABLE_MOPT=OFF`. See [MOpt integration](docs/mopt.md)
for the library boundaries, regression tests and remaining C frontend work.

Public headers live in `include/cc1`, implementations in `lib`, and the CLI in
`tools/cc1`. Tests are grouped by component under `Tests`; existing C fixtures
remain available at their original paths. See [the architecture](docs/architecture.md)
for module dependencies and AST ownership, and [semantic analysis](docs/sema.md)
for the frontend/backend contract. The [MLIR backend design](docs/mlir_backend.md)
describes the Tensorium backend and the planned MemProtect protection pipeline.

C++ formatting uses the repository's `.clang-format`. Optimization follows
`CMAKE_BUILD_TYPE`; warning settings are controlled by `CC1_ENABLE_WARNINGS`.
