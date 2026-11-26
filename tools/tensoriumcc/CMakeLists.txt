add_executable(tensoriumcc main.cpp)
set_target_properties(tensoriumcc PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")
target_link_libraries(tensoriumcc PRIVATE cc1_driver cc1_project_options cc1_warnings)
add_dependencies(tensoriumcc cc1 tensorium-opt)
