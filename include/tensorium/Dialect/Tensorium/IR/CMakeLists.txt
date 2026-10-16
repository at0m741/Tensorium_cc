set(LLVM_TARGET_DEFINITIONS TensoriumDialect.td)

mlir_tablegen(TensoriumDialect.h.inc
  -gen-dialect-decls
  -dialect=tensorium
  EXTRA_INCLUDES ${MLIR_INCLUDE_DIRS}
)

mlir_tablegen(TensoriumDialect.cpp.inc
  -gen-dialect-defs
  -dialect=tensorium
  EXTRA_INCLUDES ${MLIR_INCLUDE_DIRS}
)

add_public_tablegen_target(TensoriumDialectIncGen)

set(LLVM_TARGET_DEFINITIONS TensoriumTypes.td)

mlir_tablegen(TensoriumTypes.h.inc
  -gen-typedef-decls
  -typedefs-dialect=tensorium
  EXTRA_INCLUDES ${MLIR_INCLUDE_DIRS}
)

mlir_tablegen(TensoriumTypes.cpp.inc
  -gen-typedef-defs
  -typedefs-dialect=tensorium
  EXTRA_INCLUDES ${MLIR_INCLUDE_DIRS}
)

add_public_tablegen_target(TensoriumTypesIncGen)

set(LLVM_TARGET_DEFINITIONS TensoriumOps.td)

mlir_tablegen(TensoriumOps.h.inc
  -gen-op-decls
  EXTRA_INCLUDES ${MLIR_INCLUDE_DIRS}
)

mlir_tablegen(TensoriumOps.cpp.inc
  -gen-op-defs
  EXTRA_INCLUDES ${MLIR_INCLUDE_DIRS}
)

add_public_tablegen_target(TensoriumOpsIncGen)

