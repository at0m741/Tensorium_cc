add_executable(cc1_driver_fail_tool FailTool.cpp)
target_link_libraries(cc1_driver_fail_tool PRIVATE cc1_project_options cc1_warnings)
find_program(CC1_DRIVER_AR NAMES llvm-ar ar HINTS ${LLVM_TOOLS_BINARY_DIR})
if(Python3_Interpreter_FOUND AND CC1_MLIR_TRANSLATE AND CC1_RUNTIME_CLANG)
  function(cc1_add_driver_case executable case)
    if(executable STREQUAL "cc1")
      set(name driver_cc1_${case})
    else()
      set(name driver_${case})
    endif()
    set(archive_args)
    if(CC1_DRIVER_AR)
      list(APPEND archive_args --ar "${CC1_DRIVER_AR}")
    endif()
    add_test(NAME ${name}
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/DriverTests.py
        --driver $<TARGET_FILE:${executable}>
        --cc1 $<TARGET_FILE:cc1>
        --optimizer $<TARGET_FILE:tensorium-opt>
        --translator ${CC1_MLIR_TRANSLATE}
        --clang ${CC1_RUNTIME_CLANG}
        --fail-tool $<TARGET_FILE:cc1_driver_fail_tool>
        ${archive_args}
        --fixtures ${CMAKE_CURRENT_SOURCE_DIR}/fixtures
        --scratch ${CMAKE_CURRENT_BINARY_DIR}/scratch --case ${case})
    set_tests_properties(${name} PROPERTIES TIMEOUT 60)
  endfunction()
  foreach(executable IN ITEMS cc1 tensoriumcc)
    foreach(case IN ITEMS multi_source object_link assembly default_outputs paths
                          errors failures help optimization preflight sibling_tools)
      cc1_add_driver_case(${executable} ${case})
    endforeach()
    if(CC1_DRIVER_AR)
      cc1_add_driver_case(${executable} archive)
    endif()
  endforeach()
  cc1_add_driver_case(cc1 frontend_modes)
endif()
