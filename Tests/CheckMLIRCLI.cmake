cmake_minimum_required(VERSION 3.22)
file(MAKE_DIRECTORY "${SCRATCH_DIR}")
file(WRITE "${SCRATCH_DIR}/empty.c" "/* Empty translation unit. */\n")
file(WRITE "${SCRATCH_DIR}/valid.c" "int add(int a, int b) { return a + b; }\n")
file(WRITE "${SCRATCH_DIR}/invalid.c" "int f(void) { return missing; }\n")
file(WRITE "${SCRATCH_DIR}/locals.c" "int add(int a, int b) { return a + b; }\nint main(void) { int a = 1; int b = 4; add(a, b); }\n")

function(run_case expected_status)
  execute_process(
    COMMAND "${CC1}" -fno-color-diagnostics ${ARGN}
    WORKING_DIRECTORY "${SCRATCH_DIR}"
    RESULT_VARIABLE STATUS OUTPUT_VARIABLE OUTPUT ERROR_VARIABLE DIAGNOSTIC
    TIMEOUT 4
  )
  if(NOT "${STATUS}" STREQUAL "${expected_status}")
    message(FATAL_ERROR "Expected status ${expected_status}, got ${STATUS}: ${ARGN}\n${OUTPUT}\n${DIAGNOSTIC}")
  endif()
  set(OUTPUT "${OUTPUT}" PARENT_SCOPE)
  set(DIAGNOSTIC "${DIAGNOSTIC}" PARENT_SCOPE)
endfunction()

# Enabling the backend at build time must not enable emission for ordinary inputs.
foreach(SOURCE IN ITEMS empty.c valid.c locals.c)
  run_case(0 "${SOURCE}")
  if(NOT "${OUTPUT}" STREQUAL "" OR NOT "${DIAGNOSTIC}" STREQUAL "")
    message(FATAL_ERROR "Default invocation ran MLIR generation:\n${OUTPUT}\n${DIAGNOSTIC}")
  endif()
endforeach()

if(MLIR_ENABLED)
  run_case(0 --emit-mlir empty.c)
  string(STRIP "${OUTPUT}" MODULE)
  if(NOT "${MODULE}" MATCHES "^module \\{[ \t\r\n]*\\}$" OR NOT "${DIAGNOSTIC}" STREQUAL "")
    message(FATAL_ERROR "Incorrect empty MLIR module:\n${OUTPUT}\n${DIAGNOSTIC}")
  endif()

  run_case(0 --emit-mlir valid.c)
  if(NOT "${OUTPUT}" MATCHES "func.func @add" OR NOT "${OUTPUT}" MATCHES "arith.addi" OR NOT "${OUTPUT}" MATCHES "(func\\.)?return" OR NOT "${DIAGNOSTIC}" STREQUAL "")
    message(FATAL_ERROR "Incorrect function MLIR:\n${OUTPUT}\n${DIAGNOSTIC}")
  endif()

  run_case(0 --emit-mlir locals.c)
  if(NOT "${OUTPUT}" MATCHES "memref.alloca" OR NOT "${OUTPUT}" MATCHES "memref.store" OR NOT "${OUTPUT}" MATCHES "memref.load" OR NOT "${OUTPUT}" MATCHES "(func\\.)?call @add" OR NOT "${DIAGNOSTIC}" STREQUAL "")
    message(FATAL_ERROR "Incorrect local storage or call MLIR:\n${OUTPUT}\n${DIAGNOSTIC}")
  endif()

  file(WRITE "${SCRATCH_DIR}/unsupported.c" "int f(int x) { return x / 2; }\n")
  run_case(1 --emit-mlir unsupported.c)
  if(NOT "${OUTPUT}" STREQUAL "" OR NOT "${DIAGNOSTIC}" MATCHES "MLIR generation does not support this binary operator yet")
    message(FATAL_ERROR "Incorrect unsupported-operator diagnostic:\n${OUTPUT}\n${DIAGNOSTIC}")
  endif()

  run_case(1 --emit-mlir invalid.c)
  if(NOT "${OUTPUT}" STREQUAL "" OR NOT "${DIAGNOSTIC}" MATCHES "use of undeclared identifier 'missing'" OR "${DIAGNOSTIC}" MATCHES "MLIR generation")
    message(FATAL_ERROR "Semantic error did not stop MLIR emission:\n${OUTPUT}\n${DIAGNOSTIC}")
  endif()

  foreach(OPTION IN ITEMS -E --dump-tokens --dump-ast --dump-sema)
    foreach(ORDER IN ITEMS before after)
      if(ORDER STREQUAL "before")
        run_case(1 "${OPTION}" --emit-mlir empty.c)
      else()
        run_case(1 --emit-mlir "${OPTION}" empty.c)
      endif()
      if(NOT "${OUTPUT}" STREQUAL "" OR NOT "${DIAGNOSTIC}" MATCHES "cannot be combined")
        message(FATAL_ERROR "Conflicting output options were not rejected:\n${OUTPUT}\n${DIAGNOSTIC}")
      endif()
    endforeach()
  endforeach()
else()
  run_case(1 --emit-mlir empty.c)
  if(NOT "${OUTPUT}" STREQUAL "" OR NOT "${DIAGNOSTIC}" MATCHES "MLIR support is disabled; rebuild with -DCC1_ENABLE_MLIR=ON")
    message(FATAL_ERROR "Incorrect disabled-MLIR diagnostic:\n${OUTPUT}\n${DIAGNOSTIC}")
  endif()
endif()

# After --, the same spelling denotes a filename, not an output option.
file(WRITE "${SCRATCH_DIR}/--emit-mlir" "")
run_case(0 -- --emit-mlir)
if(NOT "${OUTPUT}" STREQUAL "" OR NOT "${DIAGNOSTIC}" STREQUAL "")
  message(FATAL_ERROR "Option terminator did not preserve default behavior")
endif()
