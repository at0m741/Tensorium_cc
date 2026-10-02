add_executable(parser_tests
  ParserTests.cpp
)

target_link_libraries(parser_tests
  PRIVATE
    cc1_parser
    cc1_project_options
    cc1_warnings
)

add_test(NAME parser_tests COMMAND parser_tests)
