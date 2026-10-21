cmake_minimum_required(VERSION 3.22)
file(MAKE_DIRECTORY "${SCRATCH_DIR}")

function(run_step)
  execute_process(
    COMMAND ${ARGN}
    RESULT_VARIABLE STATUS OUTPUT_VARIABLE OUTPUT ERROR_VARIABLE DIAGNOSTIC
    TIMEOUT 10
  )
  if(NOT "${STATUS}" STREQUAL "0")
    message(FATAL_ERROR "Failed: ${ARGN}\n${OUTPUT}\n${DIAGNOSTIC}")
  endif()
  set(OUTPUT "${OUTPUT}" PARENT_SCOPE)
endfunction()

if(INPUT_IS_MLIR)
  file(READ "${SOURCE}" OUTPUT)
else()
  run_step("${CC1}" -fno-color-diagnostics --emit-mlir "${SOURCE}")
endif()
file(WRITE "${SCRATCH_DIR}/for.mlir" "${OUTPUT}")

run_step("${TENSORIUM_OPT}" "${SCRATCH_DIR}/for.mlir"
  --convert-tensorium-to-memref -o "${SCRATCH_DIR}/for-memref.mlir")

run_step("${MLIR_OPT}" "${SCRATCH_DIR}/for-memref.mlir"
  --convert-arith-to-llvm --convert-cf-to-llvm --convert-func-to-llvm
  --finalize-memref-to-llvm --reconcile-unrealized-casts
  -o "${SCRATCH_DIR}/for-llvm.mlir")
run_step("${MLIR_TRANSLATE}" --mlir-to-llvmir
  "${SCRATCH_DIR}/for-llvm.mlir" -o "${SCRATCH_DIR}/for.ll")

# Repeat at both optimization levels so correctness does not rely on cleanup.
set(CLANG_INPUTS "${SCRATCH_DIR}/for.ll")
if(DEFINED RUNNER AND NOT "${RUNNER}" STREQUAL "")
  list(APPEND CLANG_INPUTS "${RUNNER}")
endif()
foreach(LEVEL IN ITEMS 0 2)
  set(EXECUTABLE "${SCRATCH_DIR}/for-O${LEVEL}${CMAKE_EXECUTABLE_SUFFIX}")
  run_step("${CLANG}" "-O${LEVEL}" ${CLANG_INPUTS} -o "${EXECUTABLE}")
  run_step("${EXECUTABLE}")
endforeach()
