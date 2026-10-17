add_executable(tensorium-opt tensorium-opt.cpp)
set_target_properties(tensorium-opt PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}"
)
target_link_libraries(tensorium-opt PRIVATE
  tensorium_to_memref
  MLIRMlirOptMain
  MLIRArithDialect
  MLIRControlFlowDialect
  MLIRFuncDialect
  MLIRMemRefDialect
  MLIRTransforms
)
