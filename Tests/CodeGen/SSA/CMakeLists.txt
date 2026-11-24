add_executable(cc1_local_ssa_tests LocalSSATests.cpp)
target_compile_definitions(cc1_local_ssa_tests
  PRIVATE CC1_SSA_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)
target_link_libraries(cc1_local_ssa_tests PRIVATE
  cc1_codegen cc1_parse cc1_sema cc1_project_options cc1_warnings
)
add_test(NAME mlir_local_ssa COMMAND cc1_local_ssa_tests)
set_tests_properties(mlir_local_ssa PROPERTIES TIMEOUT 10)

if(CC1_MLIR_OPT AND CC1_MLIR_TRANSLATE AND CC1_RUNTIME_CLANG)
  add_test(NAME mlir_parameter_ssa_runtime
    COMMAND ${CMAKE_COMMAND}
      -DCC1=$<TARGET_FILE:cc1>
      -DTENSORIUM_OPT=$<TARGET_FILE:tensorium-opt>
      -DMLIR_OPT=${CC1_MLIR_OPT}
      -DMLIR_TRANSLATE=${CC1_MLIR_TRANSLATE}
      -DCLANG=${CC1_RUNTIME_CLANG}
      -DUSE_LLVM_MEMORY=ON
      -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/parameters.c
      -DSCRATCH_DIR=${CMAKE_CURRENT_BINARY_DIR}/parameters-runtime
      -P ${PROJECT_SOURCE_DIR}/Tests/CheckMLIRForRuntime.cmake
  )
  set_tests_properties(mlir_parameter_ssa_runtime PROPERTIES TIMEOUT 30)

  foreach(FIXTURE IN ITEMS locals locals_control_flow locals_memory)
    add_test(NAME mlir_ssa_${FIXTURE}_runtime
      COMMAND ${CMAKE_COMMAND}
        -DCC1=$<TARGET_FILE:cc1>
        -DTENSORIUM_OPT=$<TARGET_FILE:tensorium-opt>
        -DMLIR_OPT=${CC1_MLIR_OPT}
        -DMLIR_TRANSLATE=${CC1_MLIR_TRANSLATE}
        -DCLANG=${CC1_RUNTIME_CLANG}
        -DUSE_LLVM_MEMORY=ON
        -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/${FIXTURE}.c
        -DSCRATCH_DIR=${CMAKE_CURRENT_BINARY_DIR}/${FIXTURE}-runtime
        -P ${PROJECT_SOURCE_DIR}/Tests/CheckMLIRForRuntime.cmake
    )
    set_tests_properties(mlir_ssa_${FIXTURE}_runtime PROPERTIES TIMEOUT 30)
  endforeach()
endif()
