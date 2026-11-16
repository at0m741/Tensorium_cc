mopt_add_library(mopt_analysis
  LoopAnalysis.cpp
  ReductionAnalysis.cpp
  DependenceAnalysis.cpp
  LegalityAnalysis.cpp
)
target_link_libraries(mopt_analysis PUBLIC
  MLIRIR MLIRSCFDialect MLIRMemRefDialect MLIRArithDialect
  MLIRSideEffectInterfaces
)
