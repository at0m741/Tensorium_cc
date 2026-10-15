add_mlir_dialect_library(tensorium_ir
  TensoriumDialect.cpp
  TensoriumTypes.cpp

  DEPENDS
    TensoriumDialectIncGen
    TensoriumTypesIncGen

  LINK_LIBS PUBLIC
    MLIRIR
)

target_compile_features(tensorium_ir PUBLIC cxx_std_17)

target_include_directories(tensorium_ir PUBLIC
  $<BUILD_INTERFACE:${TENSORIUM_SOURCE_INCLUDE_DIR}>
  $<BUILD_INTERFACE:${TENSORIUM_BINARY_INCLUDE_DIR}>
)

target_include_directories(tensorium_ir SYSTEM PUBLIC
  ${LLVM_INCLUDE_DIRS}
  ${MLIR_INCLUDE_DIRS}
)
