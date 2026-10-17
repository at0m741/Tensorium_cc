add_mlir_conversion_library(tensorium_to_memref
  TensoriumToMemRef.cpp

  LINK_LIBS PUBLIC
    tensorium_ir
    MLIRMemRefDialect
    MLIRFuncDialect
    MLIRControlFlowDialect
    MLIRFuncTransforms
    MLIRTransformUtils
    MLIRPass
)

target_compile_features(tensorium_to_memref PUBLIC cxx_std_17)
