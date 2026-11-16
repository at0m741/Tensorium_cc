add_executable(tensorium-opt tensorium-opt.cpp)
set_target_properties(tensorium-opt PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}"
)
target_link_libraries(tensorium-opt PRIVATE
  tensorium_to_memref
  tensorium_to_llvm
  MLIRMlirOptMain
  MLIRArithDialect
  MLIRControlFlowDialect
  MLIRFuncDialect
  MLIRMemRefDialect
  MLIRTransforms
)
if(CC1_ENABLE_MOPT)
  target_link_libraries(tensorium-opt PRIVATE mopt_passes MLIRSCFDialect)
  target_compile_definitions(tensorium-opt PRIVATE TENSORIUM_ENABLE_MOPT=1)
endif()
