find_program(TENSORIUM_DRIVER_CLANG NAMES clang HINTS ${LLVM_TOOLS_BINARY_DIR})
if(NOT TENSORIUM_DRIVER_CLANG)
  set(TENSORIUM_DRIVER_CLANG "")
endif()
configure_file(DriverConfig.hpp.in
  ${PROJECT_BINARY_DIR}/generated/cc1/Driver/DriverConfig.hpp @ONLY)

cc1_add_module(cc1_driver DriverOptions.cpp Toolchain.cpp Driver.cpp)
target_include_directories(cc1_driver PRIVATE ${PROJECT_BINARY_DIR}/generated)
target_include_directories(cc1_driver SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS})
target_link_libraries(cc1_driver PRIVATE LLVMSupport)
