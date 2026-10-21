cmake_minimum_required(VERSION 3.22)

execute_process(
  COMMAND "${CC1}" -fno-color-diagnostics --emit-mlir "${SOURCE}"
  RESULT_VARIABLE STATUS
  OUTPUT_VARIABLE OUTPUT
  ERROR_VARIABLE DIAGNOSTIC
  TIMEOUT 4
)

if(NOT "${STATUS}" STREQUAL "${EXPECTED_STATUS}")
  message(FATAL_ERROR
    "${SOURCE}: expected exit status ${EXPECTED_STATUS}, got ${STATUS}\n${DIAGNOSTIC}")
endif()

if("${EXPECTED_STATUS}" STREQUAL "0")
  if(NOT "${OUTPUT}" MATCHES "module" OR NOT "${DIAGNOSTIC}" STREQUAL "")
    message(FATAL_ERROR "Expected MLIR without diagnostics:\n${OUTPUT}\n${DIAGNOSTIC}")
  endif()
else()
  if(NOT "${OUTPUT}" STREQUAL "")
    message(FATAL_ERROR "Failed compilation emitted partial MLIR:\n${OUTPUT}")
  endif()
  if(NOT DEFINED EXPECTED_DIAGNOSTIC OR "${EXPECTED_DIAGNOSTIC}" STREQUAL "")
    message(FATAL_ERROR "A failing fixture needs EXPECTED_DIAGNOSTIC")
  endif()
  string(FIND "${DIAGNOSTIC}" "${EXPECTED_DIAGNOSTIC}" MESSAGE_POSITION)
  string(FIND "${DIAGNOSTIC}" "${SOURCE}:" SOURCE_POSITION)
  if(MESSAGE_POSITION EQUAL -1 OR SOURCE_POSITION EQUAL -1 OR
     NOT "${DIAGNOSTIC}" MATCHES "error:")
    message(FATAL_ERROR "Expected diagnostic '${EXPECTED_DIAGNOSTIC}':\n${DIAGNOSTIC}")
  endif()
endif()
