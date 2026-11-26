cmake_minimum_required(VERSION 3.22)
execute_process(
  COMMAND "${TENSORIUM_OPT}" "${SOURCE}" --convert-tensorium-to-llvm
  RESULT_VARIABLE STATUS OUTPUT_VARIABLE OUTPUT ERROR_VARIABLE DIAGNOSTIC
  TIMEOUT 10)
if(NOT "${STATUS}" STREQUAL "${EXPECTED_STATUS}")
  message(FATAL_ERROR "Expected ${EXPECTED_STATUS}, got ${STATUS}:\n${OUTPUT}\n${DIAGNOSTIC}")
endif()
if(NOT "${EXPECTED_STATUS}" STREQUAL "0")
  string(FIND "${DIAGNOSTIC}" "${EXPECTED_DIAGNOSTIC}" POSITION)
  if(POSITION EQUAL -1 OR NOT "${OUTPUT}" STREQUAL "")
    message(FATAL_ERROR "Incorrect rejection:\n${OUTPUT}\n${DIAGNOSTIC}")
  endif()
  return()
endif()
if("${OUTPUT}" MATCHES "tensorium\\.|!tensorium|unrealized_conversion_cast" OR
   NOT "${OUTPUT}" MATCHES "llvm.func" OR NOT "${DIAGNOSTIC}" STREQUAL "")
  message(FATAL_ERROR "Conversion did not produce clean LLVM IR:\n${OUTPUT}\n${DIAGNOSTIC}")
endif()
if(SOURCE MATCHES "layout(32|64)\\.mlir$")
  set(BITS "${CMAKE_MATCH_1}")
  if(NOT "${OUTPUT}" MATCHES "llvm.ptrtoint[^\n]*to i${BITS}" OR
     NOT "${OUTPUT}" MATCHES "llvm.getelementptr" OR
     NOT "${OUTPUT}" MATCHES "!llvm.array<3 x i32>")
    message(FATAL_ERROR "Incorrect target layout/array conversion:\n${OUTPUT}")
  endif()
endif()
if(SOURCE MATCHES "global_address(32|64)\\.mlir$")
  if(NOT OUTPUT MATCHES "llvm.mlir.addressof @external" OR
     NOT OUTPUT MATCHES "llvm.mlir.addressof @hidden" OR
     NOT OUTPUT MATCHES "llvm.mlir.addressof @pointer" OR
     OUTPUT MATCHES "llvm.store|llvm.ptrtoint")
    message(FATAL_ERROR "Expected symbolic LLVM address initializers: ${OUTPUT}")
  endif()
endif()
file(MAKE_DIRECTORY "${SCRATCH_DIR}")
file(WRITE "${SCRATCH_DIR}/lowered.mlir" "${OUTPUT}")
execute_process(
  COMMAND "${TENSORIUM_OPT}" "${SCRATCH_DIR}/lowered.mlir" --convert-tensorium-to-llvm
  RESULT_VARIABLE ROUNDTRIP_STATUS OUTPUT_VARIABLE ROUNDTRIP ERROR_VARIABLE ROUNDTRIP_ERROR
  TIMEOUT 10)
if(NOT "${ROUNDTRIP_STATUS}" STREQUAL "0" OR NOT "${ROUNDTRIP}" STREQUAL "${OUTPUT}")
  message(FATAL_ERROR "LLVM conversion is not parseable/idempotent:\n${ROUNDTRIP}\n${ROUNDTRIP_ERROR}")
endif()
