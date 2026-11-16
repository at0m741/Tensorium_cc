function(cc1_add_memory_program fixture)
  set(SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/valid/${fixture}.c")
  add_test(NAME mlir_memory_${fixture}
    COMMAND ${CMAKE_COMMAND}
      -DCC1=$<TARGET_FILE:cc1>
      -DSOURCE=${SOURCE}
      -DEXPECTED_STATUS=0
      -P ${PROJECT_SOURCE_DIR}/Tests/CheckMLIRFixture.cmake
  )
  set_tests_properties(mlir_memory_${fixture} PROPERTIES TIMEOUT 5)

  if(CC1_MLIR_OPT AND CC1_MLIR_TRANSLATE AND CC1_RUNTIME_CLANG)
    add_test(NAME mlir_memory_${fixture}_runtime
      COMMAND ${CMAKE_COMMAND}
        -DCC1=$<TARGET_FILE:cc1>
        -DTENSORIUM_OPT=$<TARGET_FILE:tensorium-opt>
        -DMLIR_OPT=${CC1_MLIR_OPT}
        -DMLIR_TRANSLATE=${CC1_MLIR_TRANSLATE}
        -DCLANG=${CC1_RUNTIME_CLANG}
        -DUSE_LLVM_MEMORY=ON
        -DRUNNER=${ARGN}
        -DSOURCE=${SOURCE}
        -DSCRATCH_DIR=${CMAKE_CURRENT_BINARY_DIR}/${fixture}-runtime
        -P ${PROJECT_SOURCE_DIR}/Tests/CheckMLIRForRuntime.cmake
    )
    set_tests_properties(mlir_memory_${fixture}_runtime PROPERTIES TIMEOUT 30)
  endif()
endfunction()

function(cc1_add_memory_rejection fixture diagnostic)
  add_test(NAME mlir_memory_reject_${fixture}
    COMMAND ${CMAKE_COMMAND}
      -DCC1=$<TARGET_FILE:cc1>
      -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/invalid/${fixture}.c
      -DEXPECTED_STATUS=1
      "-DEXPECTED_DIAGNOSTIC=${diagnostic}"
      -P ${PROJECT_SOURCE_DIR}/Tests/CheckMLIRFixture.cmake
  )
  set_tests_properties(mlir_memory_reject_${fixture} PROPERTIES TIMEOUT 5)
endfunction()

foreach(FIXTURE IN ITEMS address_dereference pointer_aliasing pointer_control_flow
                         pointer_parameters pointer_short local_pointer
                         pointer_assignment pointer_address nested_pointer
                         pointer_rebinding pointer_arithmetic pointer_array
                         multidimensional_array pointer_null pointer_updates
                         address_dereference2 pointer_compound)
  cc1_add_memory_program(${FIXTURE})
endforeach()

cc1_add_memory_program(pointer_external ${CMAKE_CURRENT_SOURCE_DIR}/PointerExternalRuntime.c)

cc1_add_memory_rejection(void_pointer "only pointers to supported object types")
cc1_add_memory_rejection(volatile_pointee "volatile pointees")
cc1_add_memory_rejection(const_write "assignment requires a modifiable lvalue")
