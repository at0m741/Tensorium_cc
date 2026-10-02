add_subdirectory(parser)

add_executable(cc1_diagnostic_tests DiagnosticTests.cpp)
target_link_libraries(cc1_diagnostic_tests
  PRIVATE cc1_lexer cc1_project_options cc1_warnings
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
                         nested_blocks expression_statements prototype_and_definition)
  add_test(NAME cli_${FIXTURE}
    COMMAND ${CMAKE_COMMAND}
      -DCC1=$<TARGET_FILE:cc1>
      -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/${FIXTURE}.c
      -DEXPECTED_STATUS=0
      -P ${CMAKE_CURRENT_SOURCE_DIR}/CheckParserCLI.cmake
  )
  set_tests_properties(cli_${FIXTURE} PROPERTIES TIMEOUT 5)
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

add_test(NAME cli_dump_tokens
  COMMAND ${CMAKE_COMMAND}
    -DCC1=$<TARGET_FILE:cc1>
    -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/invalid_missing_semicolon.c
    -DEXPECTED_STATUS=0
    -DDUMP_TOKENS=ON
    -P ${CMAKE_CURRENT_SOURCE_DIR}/CheckParserCLI.cmake
)
set_tests_properties(cli_dump_tokens PROPERTIES TIMEOUT 5)
