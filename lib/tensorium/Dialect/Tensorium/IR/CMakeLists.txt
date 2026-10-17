add_mlir_dialect_library(tensorium_ir
  TensoriumDialect.cpp
  TensoriumTypes.cpp
  TensoriumOps.cpp

  DEPENDS
    TensoriumDialectIncGen
    TensoriumTypesIncGen
    TensoriumOpsIncGen

  LINK_LIBS PUBLIC
    MLIRIR
    MLIRSideEffectInterfaces
)
target_compile_features(tensorium_ir PUBLIC cxx_std_17)

target_include_directories(tensorium_ir PUBLIC
  $<BUILD_INTERFACE:${TENSORIUM_SOURCE_INCLUDE_DIR}>
)

target_include_directories(tensorium_ir SYSTEM PUBLIC
  $<BUILD_INTERFACE:${TENSORIUM_BINARY_INCLUDE_DIR}>
  ${LLVM_INCLUDE_DIRS}
  ${MLIR_INCLUDE_DIRS}
)
