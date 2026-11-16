cmake_minimum_required(VERSION 3.22)
file(MAKE_DIRECTORY "${SCRATCH_DIR}")

function(run_opt)
  execute_process(COMMAND "${TENSORIUM_OPT}" "${SOURCE}" ${ARGN}
    RESULT_VARIABLE STATUS OUTPUT_VARIABLE OUTPUT ERROR_VARIABLE REPORT
    TIMEOUT 10)
  if(NOT "${STATUS}" STREQUAL "0")
    message(FATAL_ERROR "tensorium-opt failed: ${OUTPUT}\n${REPORT}")
  endif()
  set(OUTPUT "${OUTPUT}" PARENT_SCOPE)
  set(REPORT "${REPORT}" PARENT_SCOPE)
endfunction()

run_opt()
set(BASELINE "${OUTPUT}")
run_opt("--mopt-analyze-loops=mcpu=generic")
if(NOT "${OUTPUT}" STREQUAL "${BASELINE}" OR
   NOT "${REPORT}" MATCHES "mopt.loop #0" OR
   NOT "${REPORT}" MATCHES "trip-count = 8" OR
   NOT "${REPORT}" MATCHES "unknown-effects = true" OR
   NOT "${REPORT}" MATCHES "decision = unknown")
  message(FATAL_ERROR "Analysis changed IR or missed unknown Tensorium memory:\n${OUTPUT}\n${REPORT}")
endif()
file(WRITE "${SCRATCH_DIR}/analyzed.mlir" "${OUTPUT}")
execute_process(
  COMMAND "${TENSORIUM_OPT}" "${SCRATCH_DIR}/analyzed.mlir"
    -o "${SCRATCH_DIR}/roundtrip.mlir"
  RESULT_VARIABLE ROUNDTRIP_STATUS ERROR_VARIABLE ROUNDTRIP_ERROR)
if(NOT "${ROUNDTRIP_STATUS}" STREQUAL "0")
  message(FATAL_ERROR "Analysis contaminated the MLIR stream:\n${ROUNDTRIP_ERROR}")
endif()

run_opt(--convert-tensorium-to-memref "--mopt-analyze-loops=mcpu=generic")
if("${OUTPUT}" MATCHES "tensorium\\." OR
   NOT "${REPORT}" MATCHES "loads = 1" OR
   NOT "${REPORT}" MATCHES "stores = 1" OR
   NOT "${REPORT}" MATCHES "unknown-effects = false" OR
   NOT "${REPORT}" MATCHES "decision = rejected" OR
   NOT "${REPORT}" MATCHES "loop-carried-dependence")
  message(FATAL_ERROR "Analysis did not consume converted memory:\n${OUTPUT}\n${REPORT}")
endif()

execute_process(
  COMMAND "${TENSORIUM_OPT}" "${SOURCE}"
    "--mopt-analyze-loops=target-triple=not-a-real-target"
  RESULT_VARIABLE STATUS OUTPUT_VARIABLE OUTPUT ERROR_VARIABLE REPORT
  TIMEOUT 10)
if(NOT "${STATUS}" STREQUAL "1" OR NOT "${OUTPUT}" STREQUAL "" OR
   NOT "${REPORT}" MATCHES "MOpt failed to query LLVM target information")
  message(FATAL_ERROR "Invalid target did not produce a clean failure:\n${OUTPUT}\n${REPORT}")
endif()
