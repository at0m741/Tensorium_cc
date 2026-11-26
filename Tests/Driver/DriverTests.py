#!/usr/bin/env python3
"""Exercise the native driver using real C fixtures and native tool processes."""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import tempfile


class Tests:
    def __init__(self, args, directory):
        self.args = args
        self.directory = Path(directory)
        self.fixtures = Path(args.fixtures)
        self.environment = dict(os.environ)
        self.temporary = self.directory / "temp"
        self.temporary.mkdir()
        self.environment["TMPDIR"] = str(self.temporary)
        self.flags = [f"--cc1={args.cc1}", f"--tensorium-opt={args.optimizer}",
                      f"--mlir-translate={args.translator}", f"--clang={args.clang}"]

    def run(self, command, status=0, contains=None):
        result = subprocess.run([str(item) for item in command], cwd=self.directory,
                                env=self.environment, text=True,
                                stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                timeout=30)
        assert result.returncode == status, (command, result.returncode,
                                              result.stdout, result.stderr)
        if contains:
            assert contains in result.stderr, (contains, result.stderr)
        return result

    def driver(self, *arguments, status=0, contains=None, configured=True):
        result = self.run([self.args.driver] + (self.flags if configured else []) +
                          [str(item) for item in arguments], status, contains)
        assert not list(self.directory.glob("*.tmp-*")), "staged output leaked"
        assert not list(self.temporary.glob("tensoriumcc-*")), "workspace leaked"
        return result

    def source(self, name):
        return self.fixtures / name

    def includes(self):
        return "-I" + str(self.fixtures / "include")

    def multi_source(self):
        result = self.driver("-v", self.includes(), self.source("main.c"), self.source("helper.c"),
                             "-o", "program")
        assert result.stderr.count('"-cc1"') == 2, result.stderr
        assert result.stderr.count('"--emit-mlir"') == 2, result.stderr
        self.run([self.directory / "program"])

    def object_link(self):
        self.driver(self.includes(), "-c", self.source("helper.c"), "-o", "helper.o")
        assert (self.directory / "helper.o").stat().st_size > 0
        self.driver(self.includes(), self.source("main.c"), "helper.o", "-o", "mixed")
        self.run([self.directory / "mixed"])
        self.driver(self.includes(), "-c", self.source("main.c"), "-o", "main.o")
        # Object-only linking must not depend on frontend/MLIR tools.
        self.driver("--cc1=/missing/cc1", "--tensorium-opt=/missing/opt",
                    "--mlir-translate=/missing/translate", "main.o", "helper.o",
                    "-o", "objects")
        self.run([self.directory / "objects"])

    def assembly(self):
        self.driver(self.includes(), "-S", self.source("main.c"), "-o", "main.s")
        assert "main" in (self.directory / "main.s").read_text()
        self.run([self.args.clang, "-c", "main.s", "-o", "main.o"])
        self.driver(self.includes(), self.source("helper.c"), "main.o", "-o", "assembly")
        self.run([self.directory / "assembly"])

    def default_outputs(self):
        self.driver(self.source("standalone.c"))
        self.run([self.directory / "a.out"])
        self.driver("-c", self.includes(), self.source("main.c"), self.source("helper.c"))
        assert (self.directory / "main.o").exists()
        assert (self.directory / "helper.o").exists()
        self.driver("main.o", "helper.o", "-oprogram")
        self.run([self.directory / "program"])
        self.driver("-S", self.source("standalone.c"))
        assert (self.directory / "standalone.s").exists()

    def archive(self):
        self.driver("-c", self.source("library.c"), "-o", "library.o")
        self.run([self.args.ar, "rcs", "libsample.a", "library.o"])
        self.driver(self.includes(), self.source("library_main.c"), "libsample.a", "-o", "archive")
        self.run([self.directory / "archive"])
        self.driver(self.includes(), self.source("library_main.c"), "-L", self.directory,
                    "-l", "sample", "-o", "library_search")
        self.run([self.directory / "library_search"])

    def paths(self):
        odd = self.directory / "source space ' $(touch SHELL_MARKER);`touch MARKER2`.c"
        shutil.copy2(self.source("standalone.c"), odd)
        destination = self.directory / "result space ' $(touch OUTPUT_MARKER)"
        self.driver(odd, "-o", destination)
        self.run([destination])
        for marker in ("SHELL_MARKER", "MARKER2", "OUTPUT_MARKER"):
            assert not (self.directory / marker).exists()
        shutil.copy2(self.source("standalone.c"), self.directory / "-leading.c")
        self.driver("-c", "--", "-leading.c")
        assert (self.directory / "-leading.o").exists()
        include = self.directory / "include with spaces"
        shutil.copytree(self.fixtures / "include", include)
        self.driver("-I", include, self.source("main.c"), self.source("helper.c"), "-o", "includes")
        self.run([self.directory / "includes"])
        # Resolve '..' after following the directory symlink, as the OS does.
        real = self.directory / "real"
        (real / "inner").mkdir(parents=True)
        (self.directory / "link").symlink_to(real / "inner", target_is_directory=True)
        shutil.copy2(self.source("standalone.c"), real / "selected.c")
        shutil.copy2(self.source("invalid.c"), self.directory / "selected.c")
        self.driver("link/../selected.c", "-o", "through_symlink")
        self.run([self.directory / "through_symlink"])

    def errors(self):
        for arguments, diagnostic in [
            ([], "no input files"),
            (["-o"], "expected an argument"),
            (["-I"], "expected an argument"),
            (["-O4"], "unsupported option"),
            (["--target=other"], "unsupported option"),
            (["input.cpp"], "unsupported input"),
            (["missing.c"], "regular file"),
            (["--clang=", self.source("standalone.c")], "empty tool path"),
            (["-c", "-S", self.source("standalone.c")], "cannot be combined"),
            (["-c", "-o", "one.o", self.source("main.c"), self.source("helper.c")], "multiple sources"),
            (["-c", "object.o"], "accept only C sources"),
            (["-o", "-", self.source("standalone.c")], "not supported"),
        ]:
            self.driver(*arguments, status=1, contains=diagnostic)

    def failures(self):
        output = self.directory / "preserved.o"
        sentinel = b"preserve the existing output\n"
        for tool in ("cc1", "tensorium-opt", "mlir-translate", "clang"):
            output.write_bytes(sentinel)
            self.driver(f"--{tool}={self.args.fail_tool}", "-c", self.source("standalone.c"),
                        "-o", output, status=37, contains="intentional driver test tool failure")
            assert output.read_bytes() == sentinel
        # A valid C expression outside CC1's subset must also fail: no Clang C fallback.
        for name in ("invalid.c", "unsupported.c", "missing_symbol.c"):
            output.write_bytes(sentinel)
            self.driver(self.source(name), "-o", output, status=1, contains="error:")
            assert output.read_bytes() == sentinel
        for tool in ("cc1", "tensorium-opt", "mlir-translate", "clang"):
            self.driver(f"--{tool}=/definitely/missing/tool", self.source("standalone.c"),
                        "-o", "absent", status=1, contains="cannot execute tool")
            assert not (self.directory / "absent").exists()
        spaced_tool = self.directory / "failing tool with spaces"
        shutil.copy2(self.args.fail_tool, spaced_tool)
        self.driver(f"--clang={spaced_tool}", self.source("standalone.c"), status=37,
                    contains="intentional driver test tool failure")

    def help(self):
        result = self.driver("--cc1=/missing", "--help", configured=False)
        assert "Usage: " + Path(self.args.driver).name in result.stdout
        assert self.driver("--version", configured=False).stdout.startswith(Path(self.args.driver).name + " ")

    def optimization(self):
        result = self.driver("-v", "-O2", self.includes(), self.source("main.c"),
                             self.source("helper.c"), "-o", "optimized")
        for phase in ("--emit-mlir", "--convert-tensorium-to-llvm", "--mlir-to-llvmir", "-O2"):
            assert phase in result.stderr
        self.run([self.directory / "optimized"])

    def preflight(self):
        source = self.directory / "source.c"
        shutil.copy2(self.source("standalone.c"), source)
        content = source.read_bytes()
        for alias in (source, self.directory / "hard.o", self.directory / "symbolic.o"):
            if alias.name == "hard.o": os.link(source, alias)
            if alias.name == "symbolic.o": alias.symlink_to(source)
            self.driver("-c", source, "-o", alias, status=1, contains="overwrite input")
            assert source.read_bytes() == content
        first = self.directory / "first"
        second = self.directory / "second"
        first.mkdir()
        second.mkdir()
        for directory in (first, second):
            shutil.copy2(source, directory / "same.c")
        self.driver("-c", first / "same.c", second / "same.c", status=1,
                    contains="same output")
        assert not (self.directory / "same.o").exists()
        self.driver(source, "-o", first, status=1, contains="output is a directory")

    def frontend_modes(self):
        source = self.source("helper.c")
        for flags in (("-fsyntax-only",), ("-cc1",)):
            result = self.driver(*flags, self.includes(), source, configured=False)
            assert result.stdout == "" and result.stderr == ""
            assert not (self.directory / "a.out").exists()
        # Semantic checking must not invoke unsupported CodeGen operations.
        self.driver("-fsyntax-only", self.source("unsupported.c"), configured=False)
        for flags in (("--emit-mlir",), ("-cc1", "--emit-mlir")):
            result = self.driver(*flags, self.includes(), source, configured=False)
            assert "func.func @add" in result.stdout
        for flag, marker in (("--dump-ast", "TranslationUnit"),
                             ("--dump-sema", "TranslationUnit"),
                             ("--dump-tokens", "add"), ("-E", "add")):
            assert marker in self.driver(flag, self.includes(), source, configured=False).stdout
        self.driver("-fsyntax-only", "--emit-mlir", source, configured=False,
                    status=1, contains="output options cannot be combined")
        # A frontend-looking output filename is still a native -o argument.
        self.driver(self.source("standalone.c"), "-o", "--emit-mlir")
        self.run([self.directory / "--emit-mlir"])

    def write(self):
        for optimization in ("-O0", "-O2"):
            self.driver(optimization, self.source("write.c"), "-o", "write-program")
            result = self.run([self.directory / "write-program"])
            assert result.stdout == "Hi\n", result.stdout
            assert result.stderr == "", result.stderr

    def sibling_tools(self):
        # No tool overrides: exercise sibling cc1/optimizer and configured tools.
        result = self.driver("-v", self.source("standalone.c"), "-o", "sibling", configured=False)
        for name in ("cc1", "tensorium-opt"):
            assert str(Path(self.args.driver).parent / name) in result.stderr
        self.run([self.directory / "sibling"])


def main():
    parser = argparse.ArgumentParser()
    for name in ("driver", "cc1", "optimizer", "translator", "clang", "fail-tool", "fixtures", "scratch", "case"):
        parser.add_argument("--" + name, required=True)
    parser.add_argument("--ar")
    args = parser.parse_args()
    Path(args.scratch).mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix=args.case + "-", dir=args.scratch) as directory:
        getattr(Tests(args, directory), args.case)()


if __name__ == "__main__":
    main()
