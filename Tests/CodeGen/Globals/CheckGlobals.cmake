cmake_minimum_required(VERSION 3.22)
execute_process(COMMAND "${CC1}" --emit-mlir "${SOURCE}"
  RESULT_VARIABLE STATUS OUTPUT_VARIABLE OUTPUT ERROR_VARIABLE DIAGNOSTIC)
if(NOT "${STATUS}" STREQUAL "0" OR NOT "${DIAGNOSTIC}" STREQUAL "")
  message(FATAL_ERROR "Global emission failed: ${DIAGNOSTIC}")
endif()
string(REGEX MATCHALL "tensorium\\.global " GLOBALS "${OUTPUT}")
list(LENGTH GLOBALS COUNT)
if(NOT COUNT EQUAL 2 OR
   NOT OUTPUT MATCHES "tensorium.global external @counter" OR
   NOT OUTPUT MATCHES "tensorium.global internal @hidden" OR
   NOT OUTPUT MATCHES "tensorium.get_global @counter" OR
   OUTPUT MATCHES "tensorium.alloca")
  message(FATAL_ERROR "Incorrect global selection, linkage or local SSA: ${OUTPUT}")
endif()
