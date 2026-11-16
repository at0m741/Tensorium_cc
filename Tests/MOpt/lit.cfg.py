import lit.formats
from lit.llvm import llvm_config
from lit.llvm.subst import FindTool, ToolSubst

config.name = "MOpt"
config.test_format = lit.formats.ShTest(not llvm_config.use_lit_shell)
config.suffixes = [".mlir"]
config.excludes = ["Integration"]
config.test_source_root = config.mopt_source_root
config.test_exec_root = config.mopt_test_exec_root

llvm_config.with_system_environment(["HOME", "INCLUDE", "LIB", "TMP", "TEMP"])
llvm_config.use_default_substitutions()
llvm_config.add_tool_substitutions(
    [
        ToolSubst("%mopt", command=FindTool("mopt"), unresolved="fatal"),
        ToolSubst("FileCheck", unresolved="fatal"),
        ToolSubst("not", unresolved="fatal"),
    ],
    [config.mopt_tools_dir, config.filecheck_dir, config.not_dir],
)
