add_executable(mopt mopt.cpp)
set_target_properties(mopt PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}"
)
target_compile_features(mopt PRIVATE cxx_std_17)
llvm_update_compile_flags(mopt)
target_link_libraries(mopt PRIVATE
  mopt_passes MLIRParser MLIRFuncDialect
)
