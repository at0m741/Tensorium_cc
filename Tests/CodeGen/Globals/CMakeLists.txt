function(cc1_add_global_program fixture)
  add_test(NAME mlir_global_${fixture}
    COMMAND ${CMAKE_COMMAND}
      -DCC1=$<TARGET_FILE:cc1>
      -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/valid/${fixture}.c
      -DEXPECTED_STATUS=0
      -P ${PROJECT_SOURCE_DIR}/Tests/CheckMLIRFixture.cmake)
  if(CC1_MLIR_OPT AND CC1_MLIR_TRANSLATE AND CC1_RUNTIME_CLANG)
    add_test(NAME mlir_global_${fixture}_runtime
      COMMAND ${CMAKE_COMMAND}
        -DCC1=$<TARGET_FILE:cc1>
        -DTENSORIUM_OPT=$<TARGET_FILE:tensorium-opt>
        -DMLIR_TRANSLATE=${CC1_MLIR_TRANSLATE}
        -DCLANG=${CC1_RUNTIME_CLANG}
        -DUSE_LLVM_MEMORY=ON
        -DRUNNER=${ARGN}
        -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/valid/${fixture}.c
        -DSCRATCH_DIR=${CMAKE_CURRENT_BINARY_DIR}/${fixture}-runtime
        -P ${PROJECT_SOURCE_DIR}/Tests/CheckMLIRForRuntime.cmake)
    set_tests_properties(mlir_global_${fixture}_runtime PROPERTIES TIMEOUT 30)
  endif()
endfunction()
foreach(FIXTURE IN ITEMS integer_initializer declarations constant_expressions zero_storage)
  cc1_add_global_program(${FIXTURE})
endforeach()
cc1_add_global_program(external ${CMAKE_CURRENT_SOURCE_DIR}/ExternalRuntime.c)

function(cc1_reject_global fixture diagnostic)
  add_test(NAME mlir_global_reject_${fixture}
    COMMAND ${CMAKE_COMMAND}
      -DCC1=$<TARGET_FILE:cc1>
      -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/invalid/${fixture}.c
      -DEXPECTED_STATUS=1
      "-DEXPECTED_DIAGNOSTIC=${diagnostic}"
      -P ${PROJECT_SOURCE_DIR}/Tests/CheckMLIRFixture.cmake)
endfunction()
cc1_reject_global(duplicate_definition "redefinition of global object")
cc1_reject_global(nonconstant "supported integer constant expression")
cc1_reject_global(overflow32 "signed overflow in global initializer")
cc1_reject_global(overflow64 "signed overflow in global initializer")
cc1_reject_global(address_initializer "initializers currently require an integer object")
cc1_reject_global(const_write "modifiable lvalue")
cc1_reject_global(volatile "volatile globals")
cc1_reject_global(linkage "conflicting declaration")

add_test(NAME mlir_global_structure
  COMMAND ${CMAKE_COMMAND}
    -DCC1=$<TARGET_FILE:cc1>
    -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/valid/declarations.c
    -P ${CMAKE_CURRENT_SOURCE_DIR}/CheckGlobals.cmake)
