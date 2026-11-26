add_executable(tensorium_conversion_tests TensoriumToMemRefTests.cpp)
target_link_libraries(tensorium_conversion_tests PRIVATE
  tensorium_to_memref MLIRParser MLIRArithDialect
  cc1_project_options cc1_warnings
)
add_test(NAME tensorium_to_memref COMMAND tensorium_conversion_tests)
set_tests_properties(tensorium_to_memref PROPERTIES TIMEOUT 10)

function(tensorium_add_llvm_fixture fixture status)
  add_test(NAME tensorium_to_llvm_${fixture}
    COMMAND ${CMAKE_COMMAND}
      -DTENSORIUM_OPT=$<TARGET_FILE:tensorium-opt>
      -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/LLVM/${fixture}.mlir
      -DEXPECTED_STATUS=${status}
      "-DEXPECTED_DIAGNOSTIC=${ARGN}"
      -DSCRATCH_DIR=${CMAKE_CURRENT_BINARY_DIR}/llvm-${fixture}
      -P ${CMAKE_CURRENT_SOURCE_DIR}/CheckLLVMConversion.cmake
  )
  set_tests_properties(tensorium_to_llvm_${fixture} PROPERTIES TIMEOUT 15)
endfunction()

tensorium_add_llvm_fixture(layout32 0)
tensorium_add_llvm_fixture(layout64 0)
tensorium_add_llvm_fixture(missing_layout 1 "requires an explicit llvm.data_layout")
tensorium_add_llvm_fixture(invalid_layout 1 "data layout")
tensorium_add_llvm_fixture(zero_array 1 "array size must be positive")
tensorium_add_llvm_fixture(invalid_decay 1 "matching element pointer")
tensorium_add_llvm_fixture(invalid_predicate 1 "predicate attribute specification")
tensorium_add_llvm_fixture(type_metadata 1 "unsupported type-valued metadata")

tensorium_add_llvm_fixture(globals 0)
tensorium_add_llvm_fixture(predicates 0)
tensorium_add_llvm_fixture(global_missing 1 "undefined Tensorium global")
tensorium_add_llvm_fixture(global_type 1 "does not match the global object type")
tensorium_add_llvm_fixture(global_initializer 1 "matching integer initializer")
tensorium_add_llvm_fixture(global_internal_declaration 1 "internal linkage requires a definition")
tensorium_add_llvm_fixture(invalid_predicate_integer 1 "pointer comparison predicate")
