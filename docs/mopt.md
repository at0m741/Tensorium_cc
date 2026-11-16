# MOpt integration

MOpt was imported from MLIRArchOpt revision `b1984d9`, including its 17 MLIR
regression fixtures. The sources live in this repository and the build does not
require a sibling MLIRArchOpt checkout. The original checkout is not modified.

The MOpt milestone integrates analysis and planning. It does not add
vectorization, unrolling, tiling or memory protection. C pointer storage is now
provided separately by the [C memory implementation](memory.md).

## Layout and dependencies

```text
include/mopt/
  Analysis/           Loop descriptions, reductions, dependences and legality
  Target/             LLVM target configuration and queried properties
  Planning/           Vectorization decisions and report formatting
  Passes.h            Pass factories and registration
lib/mopt/
  Analysis/           mopt_analysis
  Target/             mopt_target
  Planning/           mopt_planning
  Passes/             mopt_passes
tools/mopt/           Standalone report tool
Tests/MOpt/           Original fixtures, API and Tensorium integration tests
```

`mopt_analysis` depends on standard MLIR dialects and interfaces, with no C AST,
frontend or Tensorium dependency. `analyzeLoop` returns a `LoopInfo` without
printing or changing IR. Its MLIR values refer to the source IR and must not be
retained after that IR is erased or changed.

`mopt_target` owns the LLVM TargetMachine/TTI queries formerly compiled directly
into the standalone executable. `mopt_planning` combines analysis results with
an explicit `TargetInfo`; analysis itself is independent of the target. Report
formatting takes an output stream and is separate from both computations.

`mopt_passes` composes these libraries. Its factory takes a target and a
caller-owned report stream, which must outlive the pass manager. It preserves
the input IR and existing analyses. Both tools link these libraries directly;
they do not execute each other.

LLVM RTTI/exception flags apply only to MOpt targets. The frontend keeps the
RTTI it needs for its AST. The LLVM target-query API has compatibility branches
for the tested LLVM 20 and LLVM 24 installations. The current target initializer
registers only LLVM's native backend; requesting another architecture can fail
even if other backends are installed.

## Build and use

MOpt defaults to enabled when `CC1_ENABLE_MLIR=ON`. It can be disabled using
`CC1_ENABLE_MOPT=OFF`; builds with MLIR disabled do not build any MOpt targets.
The same LLVM/MLIR package is used by all compiler components.

```sh
cmake -S . -B build-mlir -DCC1_ENABLE_MLIR=ON \
  -DMLIR_DIR=/usr/local/lib/cmake/mlir -DLLVM_DIR=/usr/local/lib/cmake/llvm
cmake --build build-mlir -j 2

build-mlir/mopt --dump-target-info --mcpu=generic Tests/MOpt/01_linear.mlir
build-mlir/tensorium-opt Tests/MOpt/01_linear.mlir \
  --mopt-analyze-loops='mcpu=generic' -o /tmp/analyzed.mlir
```

The standalone `mopt` preserves its original text report on stdout and supports
`--dump-ir`, `--dump-target-info`, `--target-triple`, `--mcpu` and `--mattr`.
The registered `mopt-analyze-loops` pass uses stderr for reports so that MLIR
output remains parseable. Its options are `target-triple`, `mcpu` and `mattr`.
Without explicit target options, these tools query the host. Future compiler
pipelines must pass the compilation target rather than assume it is the host.

## Tests

CTest always includes `mopt_analysis_api` and `mopt_tensorium_integration` when
MOpt and `BUILD_TESTING` are enabled. They load real MLIR fixtures and check
target-independent analysis, injected target models, IR preservation, report
streams, conservative handling of Tensorium memory, conversion to MemRef and
clean failure for an invalid target.

The 17 imported fixtures use LIT, FileCheck and LLVM's `not` utility. When these
are found, they run through both the `check-mopt` build target and the
`mopt_regressions` CTest entry. Their tool paths can be supplied explicitly:

```sh
cmake -S . -B build-mlir \
  -DMOPT_LIT_EXECUTABLE=/opt/homebrew/bin/lit \
  -DMOPT_LLVM_TEST_TOOLS_DIR=/opt/homebrew/opt/llvm@20/bin
cmake --build build-mlir --target check-mopt
ctest --test-dir build-mlir -R '^mopt_' --output-on-failure
```

Those paths select test utilities; linked LLVM/MLIR libraries still come from
the configured package. Missing test utilities skip the LIT suite with a CMake
status message. `MOPT_INCLUDE_TESTS=OFF` explicitly disables that suite while
retaining the API and integration tests. The imported vector-width expectations
currently assume a target with 128-bit fixed vectors, as in the original tests.

## Connection to C generation

CC1 currently emits `cf` branches for C loops. MOpt analyzes `scf.for`; importing
the pass does not reconstruct those loops or automatically optimize C output.
The next integration milestone is structured emission or reconstruction of
eligible C loops, with induction variables and carried values in SSA form.

MOpt recognizes indexed `memref.load`/`memref.store` accesses. Tensorium memory
operations currently cause an unknown-memory-effect result, not a proof of
independence. The scalar Tensorium-to-MemRef compatibility conversion can expose
those accesses to MOpt, but does not supply array shapes or indexing for general
C pointers. Future Tensorium-aware analyses or a suitable structured-memory
conversion are needed for that case.

Target-aware analyses and transformations belong before final lowering to LLVM.
Direct Tensorium-to-LLVM lowering is implemented for C object memory; MemProtect
remains separate planned work.
