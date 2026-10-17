add_subdirectory(Parse)

add_executable(cc1_ast_tests AST/ASTContextTests.cpp)
target_link_libraries(cc1_ast_tests
  PRIVATE cc1_parse cc1_sema cc1_project_options cc1_warnings
)
add_test(NAME ast_context COMMAND cc1_ast_tests)

add_executable(cc1_frontend_tests Frontend/FrontendTests.cpp)
target_compile_definitions(cc1_frontend_tests
  PRIVATE CC1_TEST_SOURCE_DIR="${PROJECT_SOURCE_DIR}/Tests"
)
target_link_libraries(cc1_frontend_tests
  PRIVATE cc1_frontend cc1_project_options cc1_warnings
)
add_test(NAME frontend COMMAND cc1_frontend_tests)

add_executable(cc1_preprocessor_tests Lex/PreprocessorTests.cpp)
target_compile_definitions(cc1_preprocessor_tests
  PRIVATE CC1_TEST_SOURCE_DIR="${PROJECT_SOURCE_DIR}/Tests"
)
target_link_libraries(cc1_preprocessor_tests
  PRIVATE cc1_lex cc1_project_options cc1_warnings
)
add_test(NAME preprocessor COMMAND cc1_preprocessor_tests)
set_tests_properties(preprocessor PROPERTIES TIMEOUT 10)

add_test(NAME preprocessor_cli
  COMMAND ${CMAKE_COMMAND}
    -DCC1=$<TARGET_FILE:cc1>
    -DTEST_DIR=${CMAKE_CURRENT_SOURCE_DIR}/preprocessor
    -DSCRATCH_DIR=${CMAKE_CURRENT_BINARY_DIR}/preprocessor-cli
    -P ${CMAKE_CURRENT_SOURCE_DIR}/CheckPreprocessorCLI.cmake
)
set_tests_properties(preprocessor_cli PROPERTIES TIMEOUT 10)

# Optional differential coverage; normal tests do not require Clang or Python.
find_program(CC1_REFERENCE_CLANG NAMES clang)
find_package(Python3 COMPONENTS Interpreter QUIET)
if(CC1_REFERENCE_CLANG AND Python3_Interpreter_FOUND)
  add_test(NAME preprocessor_clang_comparison
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/CompareMacroExpansion.py
      --cc1 $<TARGET_FILE:cc1>
      --clang ${CC1_REFERENCE_CLANG}
      --scratch ${CMAKE_CURRENT_BINARY_DIR}/macro-comparison
  )
  set_tests_properties(preprocessor_clang_comparison PROPERTIES TIMEOUT 20)
endif()

add_executable(cc1_diagnostic_tests Basic/DiagnosticTests.cpp)
target_link_libraries(cc1_diagnostic_tests
  PRIVATE cc1_lex cc1_project_options cc1_warnings
)
add_test(NAME diagnostics COMMAND cc1_diagnostic_tests)

add_test(NAME diagnostics_cli
  COMMAND ${CMAKE_COMMAND}
    -DCC1=$<TARGET_FILE:cc1>
    -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/fixtures/invalid_character.c
    -P ${CMAKE_CURRENT_SOURCE_DIR}/CheckDiagnostics.cmake
)
set_tests_properties(diagnostics_cli PROPERTIES TIMEOUT 5)

foreach(FIXTURE IN ITEMS function_body empty_function_body void_return
                         nested_blocks expression_statements prototype_and_definition
                         basic_decls function_pointer)
  add_test(NAME cli_${FIXTURE}
    COMMAND ${CMAKE_COMMAND}
      -DCC1=$<TARGET_FILE:cc1>
      -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/${FIXTURE}.c
      -DEXPECTED_STATUS=0
      -P ${CMAKE_CURRENT_SOURCE_DIR}/CheckParserCLI.cmake
  )
  set_tests_properties(cli_${FIXTURE} PROPERTIES TIMEOUT 5)
endforeach()

foreach(FIXTURE IN ITEMS function_body basic_decls function_pointer
                         empty_function_body expression_statements nested_blocks
                         prototype_and_definition void_return ast_grouping ast_assignments
                         ast_unary ast_conditional ast_calls ast_comma_argument ast_index
                         ast_member ast_sizeof ast_literals mixed_structs
                         ast_escaped_literals ast_sizeof_expr ast_member_dot ast_unary_ops
                         preprocessor/program preprocessor/macros)
  add_test(NAME cli_ast_${FIXTURE}
    COMMAND ${CMAKE_COMMAND}
      -DCC1=$<TARGET_FILE:cc1>
      -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/${FIXTURE}.c
      -DEXPECTED_STATUS=0
      -DDUMP_AST=ON
      -DEXPECTED_AST=${CMAKE_CURRENT_SOURCE_DIR}/${FIXTURE}.ast
      -P ${CMAKE_CURRENT_SOURCE_DIR}/CheckParserCLI.cmake
  )
  set_tests_properties(cli_ast_${FIXTURE} PROPERTIES TIMEOUT 5)
endforeach()

function(cc1_add_cli_error_test fixture line col message)
  add_test(NAME cli_${fixture}
    COMMAND ${CMAKE_COMMAND}
      -DCC1=$<TARGET_FILE:cc1>
      -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/${fixture}.c
      -DEXPECTED_STATUS=1
      "-DEXPECTED_DIAGNOSTIC=${line}:${col}: error: ${message}"
      -P ${CMAKE_CURRENT_SOURCE_DIR}/CheckParserCLI.cmake
  )
  set_tests_properties(cli_${fixture} PROPERTIES TIMEOUT 5)
endfunction()

cc1_add_cli_error_test(invalid_missing_semicolon 3 1 "expected ';' after return")
cc1_add_cli_error_test(invalid_unclosed_block 3 1 "expected '}' at end of block")
cc1_add_cli_error_test(invalid_return_expression 2 14 "expected expression")
cc1_add_cli_error_test(invalid_ternary 2 14 "expected expression")
cc1_add_cli_error_test(invalid_parameter_list 1 12 "expected parameter type")

add_test(NAME cli_ast_invalid_parameter_list
  COMMAND ${CMAKE_COMMAND}
    -DCC1=$<TARGET_FILE:cc1>
    -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/invalid_parameter_list.c
    -DEXPECTED_STATUS=1
    -DDUMP_AST=ON
    "-DEXPECTED_DIAGNOSTIC=1:12: error: expected parameter type"
    -P ${CMAKE_CURRENT_SOURCE_DIR}/CheckParserCLI.cmake
)
set_tests_properties(cli_ast_invalid_parameter_list PROPERTIES TIMEOUT 5)

add_custom_target(ast_audit
  COMMAND ${CMAKE_COMMAND}
    -DCC1=$<TARGET_FILE:cc1>
    -DOUTPUT_DIR=${PROJECT_BINARY_DIR}/ast-audit
    -P ${CMAKE_CURRENT_SOURCE_DIR}/AuditAST.cmake
  DEPENDS cc1
  COMMENT "Dumping all C fixtures and recording ASTs, diagnostics and exit statuses"
)

add_test(NAME cli_dump_tokens
  COMMAND ${CMAKE_COMMAND}
    -DCC1=$<TARGET_FILE:cc1>
    -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/invalid_missing_semicolon.c
    -DEXPECTED_STATUS=0
    -DDUMP_TOKENS=ON
    -P ${CMAKE_CURRENT_SOURCE_DIR}/CheckParserCLI.cmake
)
set_tests_properties(cli_dump_tokens PROPERTIES TIMEOUT 5)

add_executable(cc1_sema_tests Sema/SemaTests.cpp)
target_link_libraries(cc1_sema_tests
  PRIVATE cc1_parse cc1_sema cc1_project_options cc1_warnings
)
add_test(NAME sema COMMAND cc1_sema_tests)
set_tests_properties(sema PROPERTIES TIMEOUT 10)

add_test(NAME sema_cli
  COMMAND ${CMAKE_COMMAND}
    -DCC1=$<TARGET_FILE:cc1>
    -DSCRATCH_DIR=${CMAKE_CURRENT_BINARY_DIR}/sema-cli
    -P ${CMAKE_CURRENT_SOURCE_DIR}/CheckSemaCLI.cmake
)
set_tests_properties(sema_cli PROPERTIES TIMEOUT 10)

add_test(NAME mlir_cli
  COMMAND ${CMAKE_COMMAND}
    -DCC1=$<TARGET_FILE:cc1>
    -DMLIR_ENABLED=${CC1_ENABLE_MLIR}
    -DSCRATCH_DIR=${CMAKE_CURRENT_BINARY_DIR}/mlir-cli
    -P ${CMAKE_CURRENT_SOURCE_DIR}/CheckMLIRCLI.cmake
)
set_tests_properties(mlir_cli PROPERTIES TIMEOUT 10)

if(CC1_ENABLE_MLIR)
  add_subdirectory(Dialect/Tensorium)
  add_subdirectory(Conversion)

  add_executable(cc1_mlir_tests CodeGen/MLIRGenTests.cpp)
  target_link_libraries(cc1_mlir_tests
    PRIVATE cc1_codegen cc1_parse cc1_sema cc1_project_options cc1_warnings
  )
  add_test(NAME mlir_codegen COMMAND cc1_mlir_tests)
  set_tests_properties(mlir_codegen PROPERTIES TIMEOUT 10)

  # Run the generated loop as native code when the LLVM tools are installed.
  # Use the tools exported by the selected MLIR package. PATH can still contain
  # an older installation when multiple LLVM versions coexist.
  if(TARGET mlir-opt AND TARGET mlir-translate)
    set(CC1_MLIR_OPT "$<TARGET_FILE:mlir-opt>")
    set(CC1_MLIR_TRANSLATE "$<TARGET_FILE:mlir-translate>")
  else()
    find_program(CC1_MLIR_OPT NAMES mlir-opt HINTS ${LLVM_TOOLS_BINARY_DIR})
    find_program(CC1_MLIR_TRANSLATE NAMES mlir-translate
      HINTS ${LLVM_TOOLS_BINARY_DIR})
  endif()
  find_program(CC1_RUNTIME_CLANG NAMES clang HINTS ${LLVM_TOOLS_BINARY_DIR})
  if(CC1_MLIR_OPT AND CC1_MLIR_TRANSLATE AND CC1_RUNTIME_CLANG)
    add_test(NAME mlir_for_runtime
      COMMAND ${CMAKE_COMMAND}
        -DCC1=$<TARGET_FILE:cc1>
        -DTENSORIUM_OPT=$<TARGET_FILE:tensorium-opt>
        -DMLIR_OPT=${CC1_MLIR_OPT}
        -DMLIR_TRANSLATE=${CC1_MLIR_TRANSLATE}
        -DCLANG=${CC1_RUNTIME_CLANG}
        -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/For_stmt.c
        -DRUNNER=${CMAKE_CURRENT_SOURCE_DIR}/CodeGen/ForRuntime.c
        -DSCRATCH_DIR=${CMAKE_CURRENT_BINARY_DIR}/mlir-for-runtime
        -P ${CMAKE_CURRENT_SOURCE_DIR}/CheckMLIRForRuntime.cmake
    )
    set_tests_properties(mlir_for_runtime PROPERTIES TIMEOUT 30)

    add_test(NAME tensorium_memory_runtime
      COMMAND ${CMAKE_COMMAND}
        -DINPUT_IS_MLIR=ON
        -DTENSORIUM_OPT=$<TARGET_FILE:tensorium-opt>
        -DMLIR_OPT=${CC1_MLIR_OPT}
        -DMLIR_TRANSLATE=${CC1_MLIR_TRANSLATE}
        -DCLANG=${CC1_RUNTIME_CLANG}
        -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/Conversion/scalar_memory.mlir
        -DRUNNER=${CMAKE_CURRENT_SOURCE_DIR}/Conversion/ScalarMemoryRuntime.c
        -DSCRATCH_DIR=${CMAKE_CURRENT_BINARY_DIR}/tensorium-memory-runtime
        -P ${CMAKE_CURRENT_SOURCE_DIR}/CheckMLIRForRuntime.cmake
    )
    set_tests_properties(tensorium_memory_runtime PROPERTIES TIMEOUT 30)
  endif()
endif()
