cmake_minimum_required(VERSION 3.22)

set(MODE_OPTION)
if(DUMP_TOKENS)
  set(MODE_OPTION --dump-tokens)
elseif(DUMP_AST)
  set(MODE_OPTION --dump-ast)
endif()

execute_process(
  COMMAND "${CC1}" -fno-color-diagnostics ${MODE_OPTION} "${SOURCE}"
  RESULT_VARIABLE STATUS
  OUTPUT_VARIABLE OUTPUT
  ERROR_VARIABLE DIAGNOSTIC
  TIMEOUT 4
)

if(NOT "${STATUS}" STREQUAL "${EXPECTED_STATUS}")
  message(FATAL_ERROR "Expected exit status ${EXPECTED_STATUS}, got ${STATUS}\n${DIAGNOSTIC}")
endif()

if(DUMP_TOKENS)
  if(NOT "${OUTPUT}" MATCHES "KW_INT" OR NOT "${OUTPUT}" MATCHES "KW_RETURN" OR
     NOT "${OUTPUT}" MATCHES "END_OF_FILE" OR NOT "${DIAGNOSTIC}" STREQUAL "")
    message(FATAL_ERROR "Token dump did not stay in lexer-only mode:\n${OUTPUT}\n${DIAGNOSTIC}")
  endif()
elseif(DUMP_AST AND "${EXPECTED_STATUS}" STREQUAL "0")
  file(READ "${EXPECTED_AST}" EXPECTED_OUTPUT)
  if(NOT "${OUTPUT}" STREQUAL "${EXPECTED_OUTPUT}")
    message(FATAL_ERROR "Incorrect AST output:\n${OUTPUT}\nExpected:\n${EXPECTED_OUTPUT}")
  endif()
elseif(NOT "${OUTPUT}" STREQUAL "")
  message(FATAL_ERROR "Default parsing printed tokens:\n${OUTPUT}")
endif()

if(DEFINED EXPECTED_DIAGNOSTIC)
  string(FIND "${DIAGNOSTIC}" "${SOURCE}:${EXPECTED_DIAGNOSTIC}\n" HEADER)
  string(REGEX MATCHALL "error:" ERRORS "${DIAGNOSTIC}")
  list(LENGTH ERRORS ERROR_COUNT)
  if(HEADER EQUAL -1 OR NOT ERROR_COUNT EQUAL 1 OR
     NOT "${DIAGNOSTIC}" MATCHES "\\^")
    message(FATAL_ERROR "Incorrect parser diagnostic:\n${DIAGNOSTIC}")
  endif()
elseif(EXPECT_ANY_DIAGNOSTIC)
  if(NOT "${DIAGNOSTIC}" MATCHES "error:" OR NOT "${DIAGNOSTIC}" MATCHES "\\^")
    message(FATAL_ERROR "Expected a source diagnostic:\n${DIAGNOSTIC}")
  endif()
elseif(NOT "${DIAGNOSTIC}" STREQUAL "")
  message(FATAL_ERROR "Unexpected diagnostic:\n${DIAGNOSTIC}")
endif()
