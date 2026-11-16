add_executable(mopt_api_tests AnalysisTests.cpp)
target_link_libraries(mopt_api_tests PRIVATE
  mopt_passes MLIRParser MLIRFuncDialect
)
llvm_update_compile_flags(mopt_api_tests)
add_test(NAME mopt_analysis_api
  COMMAND mopt_api_tests ${CMAKE_CURRENT_SOURCE_DIR}/01_linear.mlir
)
set_tests_properties(mopt_analysis_api PROPERTIES TIMEOUT 10)

add_test(NAME mopt_tensorium_integration
  COMMAND ${CMAKE_COMMAND}
    -DTENSORIUM_OPT=$<TARGET_FILE:tensorium-opt>
    -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/Integration/tensorium_loop.mlir
    -DSCRATCH_DIR=${CMAKE_CURRENT_BINARY_DIR}/integration
    -P ${CMAKE_CURRENT_SOURCE_DIR}/CheckTensoriumIntegration.cmake
)
set_tests_properties(mopt_tensorium_integration PROPERTIES TIMEOUT 15)

option(MOPT_INCLUDE_TESTS "Enable the imported MOpt LIT regression suite" ON)
if(NOT MOPT_INCLUDE_TESTS)
  return()
endif()

set(MOPT_LIT_EXECUTABLE "" CACHE FILEPATH "Path to llvm-lit or lit")
set(MOPT_LLVM_TEST_TOOLS_DIR "" CACHE PATH
  "Directory containing FileCheck and not")

if(NOT MOPT_LIT_EXECUTABLE)
  find_program(_mopt_lit NAMES llvm-lit lit)
  if(_mopt_lit)
    set(MOPT_LIT_EXECUTABLE "${_mopt_lit}" CACHE FILEPATH
      "Path to llvm-lit or lit" FORCE)
  endif()
endif()
find_program(MOPT_FILECHECK_EXECUTABLE NAMES FileCheck
  HINTS "${MOPT_LLVM_TEST_TOOLS_DIR}" "${LLVM_TOOLS_BINARY_DIR}")
find_program(MOPT_NOT_EXECUTABLE NAMES not
  HINTS "${MOPT_LLVM_TEST_TOOLS_DIR}" "${LLVM_TOOLS_BINARY_DIR}")

# Test utilities are optional; building the frontend and the two tests above
# must not require installing a Python LIT package or LLVM's test executables.
if(NOT MOPT_LIT_EXECUTABLE OR NOT MOPT_FILECHECK_EXECUTABLE OR
   NOT MOPT_NOT_EXECUTABLE)
  message(STATUS "MOpt LIT tests skipped: set MOPT_LIT_EXECUTABLE and "
    "MOPT_LLVM_TEST_TOOLS_DIR to enable the 17 imported regressions")
  return()
endif()

get_filename_component(MOPT_FILECHECK_DIR "${MOPT_FILECHECK_EXECUTABLE}" DIRECTORY)
get_filename_component(MOPT_NOT_DIR "${MOPT_NOT_EXECUTABLE}" DIRECTORY)
configure_file(lit.site.cfg.py.in lit.site.cfg.py @ONLY)

add_custom_target(check-mopt
  COMMAND "${MOPT_LIT_EXECUTABLE}" -sv "${CMAKE_CURRENT_BINARY_DIR}"
  DEPENDS mopt
  COMMENT "Running the imported MOpt regression suite"
  USES_TERMINAL
)
add_test(NAME mopt_regressions
  COMMAND "${MOPT_LIT_EXECUTABLE}" -sv "${CMAKE_CURRENT_BINARY_DIR}"
)
set_tests_properties(mopt_regressions PROPERTIES TIMEOUT 60)
