mopt_add_library(mopt_target TargetInfo.cpp)
llvm_map_components_to_libnames(MOPT_LLVM_TARGET_LIBS
  analysis core mc nativecodegen target targetparser support
)
target_link_libraries(mopt_target PUBLIC ${MOPT_LLVM_TARGET_LIBS})
