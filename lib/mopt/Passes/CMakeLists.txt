mopt_add_library(mopt_passes LoopAnalysisPass.cpp)
target_link_libraries(mopt_passes PUBLIC
  mopt_planning mopt_target MLIRPass
)
