add_mlir_conversion_library(tensorium_to_llvm
  TensoriumToLLVM.cpp
  LINK_LIBS PUBLIC
    tensorium_ir
    MLIRLLVMCommonConversion
    MLIRLLVMDialect
    MLIRArithToLLVM
    MLIRControlFlowToLLVM
    MLIRFuncToLLVM
    MLIRPass
)
target_compile_features(tensorium_to_llvm PUBLIC cxx_std_17)
