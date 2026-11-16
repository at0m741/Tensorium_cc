# Match LLVM's RTTI/exception configuration only for MOpt targets. The C
# frontend uses C++ RTTI and must keep its own compilation settings.
function(mopt_add_library target)
  add_library(${target} STATIC ${ARGN})
  target_compile_features(${target} PUBLIC cxx_std_17)
  target_include_directories(${target} PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  )
  target_include_directories(${target} SYSTEM PUBLIC
    ${LLVM_INCLUDE_DIRS} ${MLIR_INCLUDE_DIRS}
  )
  llvm_update_compile_flags(${target})
endfunction()

add_subdirectory(Analysis)
add_subdirectory(Target)
add_subdirectory(Planning)
add_subdirectory(Passes)
