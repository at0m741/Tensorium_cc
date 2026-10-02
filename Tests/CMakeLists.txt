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
