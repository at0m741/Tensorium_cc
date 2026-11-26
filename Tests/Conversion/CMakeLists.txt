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
tensorium_add_llvm_fixture(invalid_predicate 1 "enum values for attribute 'predicate'")
tensorium_add_llvm_fixture(type_metadata 1 "unsupported type-valued metadata")

tensorium_add_llvm_fixture(globals 0)
tensorium_add_llvm_fixture(predicates 0)
tensorium_add_llvm_fixture(global_missing 1 "undefined Tensorium global")
tensorium_add_llvm_fixture(global_type 1 "does not match the global object type")
tensorium_add_llvm_fixture(global_initializer 1 "matching integer initializer")
tensorium_add_llvm_fixture(global_internal_declaration 1 "internal linkage requires a definition")
tensorium_add_llvm_fixture(global_public_declaration 1 "symbol declaration cannot have public visibility")
tensorium_add_llvm_fixture(invalid_predicate_integer 1 "pointer comparison predicate")

tensorium_add_llvm_fixture(global_address32 0)
tensorium_add_llvm_fixture(global_address64 0)
tensorium_add_llvm_fixture(global_address_missing 1 "address initializer references an undefined Tensorium global")
tensorium_add_llvm_fixture(global_address_type 1 "address initializer pointee type does not match")
tensorium_add_llvm_fixture(global_address_nonpointer 1 "address initializer requires a pointer object")
tensorium_add_llvm_fixture(global_address_function 1 "address initializer references an undefined Tensorium global")

tensorium_add_llvm_fixture(void_pointer32 0)
tensorium_add_llvm_fixture(void_pointer64 0)
tensorium_add_llvm_fixture(void_alloca 1 "cannot allocate a void object")
tensorium_add_llvm_fixture(void_load 1 "cannot load a void object")
tensorium_add_llvm_fixture(void_store 1 "cannot store a void object")
tensorium_add_llvm_fixture(void_offset 1 "cannot offset a void pointer")
tensorium_add_llvm_fixture(void_difference 1 "cannot subtract void pointers")
tensorium_add_llvm_fixture(invalid_pointer_cast 1 "identical pointees or a void pointer endpoint")
tensorium_add_llvm_fixture(void_array 1 "expected a supported array element type")

foreach(LAYOUT IN ITEMS 32 64 mixed)
  tensorium_add_llvm_fixture(global_array_address${LAYOUT} 0)
endforeach()
foreach(FIXTURE IN ITEMS root_negative root_overflow root_one_past negative overflow
                         too_deep parent_one_past)
  tensorium_add_llvm_fixture(global_array_${FIXTURE} 1 "invalid or out-of-bounds array path")
endforeach()
tensorium_add_llvm_fixture(global_array_pointer_type 1 "address initializer pointee type does not match")
tensorium_add_llvm_fixture(global_array_index_width 1 "target GEP index width")

tensorium_add_llvm_fixture(global_array_void_pointer32 0)
tensorium_add_llvm_fixture(global_array_void_pointer64 0)
tensorium_add_llvm_fixture(global_array_void_bounds 1 "invalid or out-of-bounds array path")
tensorium_add_llvm_fixture(global_array_void_parent_one_past 1 "invalid or out-of-bounds array path")
