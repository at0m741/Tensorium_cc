# Native compiler driver

With `CC1_ENABLE_MLIR=ON`, `cc1` is the native compiler command. It uses the
shared `cc1_driver` library to compile and link the supported C subset.
`tensoriumcc` remains an alternate executable for the same native driver.
Building the `cc1` target also builds the driver library and `tensorium-opt`.
No separate driver executable needs to be built or launched by the user.

## Usage

```sh
# Compile one translation unit to an object.
build-mlir/cc1 -c source.c -o source.o

# Compile and link multiple translation units.
build-mlir/cc1 -I include main.c utils.c -o program

# Mix sources with existing native objects or static archives.
build-mlir/cc1 main.c utils.o libsupport.a -o program
build-mlir/cc1 main.c -L lib -lsupport -o program

# Emit assembly or select native backend optimization.
build-mlir/cc1 -S source.c -o source.s
build-mlir/cc1 -O2 main.c utils.c -o program
```

In an MLIR-enabled build, `cc1 source.c` compiles and links an executable named
`a.out`. With `-c` or `-S`, each
source produces a basename-derived `.o` or `.s` in the current directory. An
explicit `-o` selects one output and requires one source for those actions.
Multiple sources with colliding output basenames are rejected before execution.
Object-only and archive-only link invocations require only the native backend.
Library flags retain their order relative to input objects and sources.

Supported options are `-c`, `-S`, `-o`, `-I`, `-L`, `-l`, `-v`, `--help`,
`--version`, `--`, and `-O0`, `-O1`, `-O2`, `-O3`, `-Os`, `-Oz`. The default
optimization is `-O0`. Optimization options select Clang's native backend
optimization; they do not enable new Tensorium or MOpt transformation passes.
Unrecognized options and unsupported input formats are diagnosed.

## Pipeline and tool discovery

```text
source.c -> cc1 (driver) -> cc1 -cc1 --emit-mlir
         -> tensorium-opt --convert-tensorium-to-llvm
         -> mlir-translate --mlir-to-llvmir
         -> clang -c (LLVM IR input)
         -> object.o
objects and libraries -> clang (platform linking) -> executable
```

For `-S`, the last compilation step uses `clang -S`. All C sources pass through
CC1 and Tensorium. Clang consumes the generated LLVM IR and performs native
code generation, assembly and platform linking. It does not compile the C input
as a fallback when the frontend cannot handle it.

Tool selection is deterministic:

1. An explicit `--cc1=PATH`, `--tensorium-opt=PATH`,
   `--mlir-translate=PATH` or `--clang=PATH` overrides that tool.
2. CC1 and the optimizer are sought beside the driver executable.
3. MLIR translation uses the selected LLVM package's tools directory. Clang
   uses the path discovered at configuration time; set
   `-DTENSORIUM_DRIVER_CLANG=/path/to/clang` to choose it.
4. Missing preferred locations fall back to `PATH`.

The selected CC1 tool must support the internal `-cc1` frontend mode. The
internal invocation bypasses driver dispatch, so compiling through `cc1` cannot
recursively invoke the native pipeline. The selected MLIR translation tool must be compatible with the LLVM dialect
emitted by the build. `-v` prints the actual commands. `--help` and `--version`
do not require tool discovery. Executable overrides must resolve to executable
regular files; a missing explicit override does not fall back to another tool.

## Frontend-only actions

Explicit frontend actions remain available directly from the main command:

```sh
build-mlir/cc1 -fsyntax-only source.c
build-mlir/cc1 -E source.c
build-mlir/cc1 --dump-ast source.c
build-mlir/cc1 --dump-sema source.c
build-mlir/cc1 --dump-tokens source.c
build-mlir/cc1 --emit-mlir source.c
```

These actions operate on one translation unit and do not perform native linking.
`-fsyntax-only` parses and runs Sema without invoking CodeGen. Frontend output
modes cannot be combined. Diagnostic color options work in frontend mode and are
forwarded by native compilation.

The reserved first argument `-cc1` selects the internal frontend explicitly;
`cc1 -cc1 source.c` performs semantic checking and
`cc1 -cc1 --emit-mlir source.c` emits raw MLIR. The native driver uses this entry
point as a subprocess. Without MLIR support, `cc1` retains its original default
syntax/semantic checking behavior and does not provide native output actions.

## Implementation boundaries and file handling

```text
include/cc1/Driver/DriverOptions.hpp  # Actions, inputs and command-line options
include/cc1/Driver/Toolchain.hpp      # Resolved subprocess tools
include/cc1/Driver/Driver.hpp         # Entry point
lib/Driver/DriverOptions.cpp         # Parsing and action validation
lib/Driver/Toolchain.cpp             # Tool discovery
lib/Driver/Driver.cpp                # Pipeline and output handling
tools/cc1/main.cpp                   # Native/frontend dispatch
tools/tensoriumcc/main.cpp           # Alternate native driver executable
Tests/Driver/                        # C fixtures and native integration tests
```

The driver uses LLVM Support for subprocesses and filesystem operations. It
does not share parser, semantic-analysis or CodeGen state between translation
units. Inputs are passed as argument vectors, without a shell. Paths with
spaces, shell metacharacters and leading dashes are supported; `..` retains its
normal filesystem meaning when a directory symlink is involved.

Intermediates use a unique workspace under the system temporary directory.
Final objects, assembly and executables are staged beside their destinations
and published by rename after success. Normal success/error returns clean up
intermediates and staged files. A failed stage preserves an existing output;
successful outputs from earlier inputs in a multi-source `-c` invocation remain.
Outputs that alias an explicit input, including symlink/hard-link aliases, are
rejected. The first failed subprocess stops the pipeline and its positive exit
status is propagated; execution failures/signals produce a nonzero driver error.

## Scope and validation

This is a native host-target driver. Cross-target selection, sysroot and SDK
discovery, `-D`/`-U`, `-std`, debug options, dependency files, response files,
stdin/stdout compilation, LTO, shared-library creation and installation/package
rules remain future work. System include directories are not discovered
automatically. Language and ABI limitations remain those of CC1/CodeGen; adding
the driver does not establish full C99 or hosted-library compatibility.

Tests use real `.c` and `.h` fixtures and execute generated programs. Coverage
includes multi-source linking, source/object mixtures, archives and library
search, assembly, output defaults, optimization, unusual paths, output aliases,
collisions, missing tools, failures at each stage, cleanup and output preservation.
The native cases run through both `cc1` and `tensoriumcc`; additional cases cover
frontend dispatch, internal mode and the absence of recursive driver calls.

```sh
cmake --build build-mlir -j 2
ctest --test-dir build-mlir -R '^driver_' --output-on-failure
```
