cmake_minimum_required(VERSION 3.22)

project(cc1 VERSION 0.1.0 LANGUAGES CXX)

include(CTest)

option(CC1_ENABLE_WARNINGS "Enable extra compiler warnings" ON)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CC1_INCLUDE_DIRS
  ${PROJECT_SOURCE_DIR}/include
)

add_library(cc1_project_options INTERFACE)
target_compile_features(cc1_project_options INTERFACE cxx_std_17)

add_library(cc1_warnings INTERFACE)
if(CC1_ENABLE_WARNINGS)
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(cc1_warnings INTERFACE -Wall -Wextra -Wpedantic)
  elseif(MSVC)
    target_compile_options(cc1_warnings INTERFACE /W4 /permissive-)
  endif()
endif()

function(cc1_add_module target)
  add_library(${target} STATIC ${ARGN})
  target_include_directories(${target}
    PUBLIC
      ${CC1_INCLUDE_DIRS}
  )
  target_link_libraries(${target}
    PUBLIC cc1_project_options
    PRIVATE cc1_warnings
  )
endfunction()

cc1_add_module(cc1_basic
  lib/Basic/Diagnostic.cpp
  lib/Basic/SourceManager.cpp
  lib/Basic/TargetInfo.cpp
)

cc1_add_module(cc1_ast
  lib/AST/Type.cpp
  lib/AST/ASTDump.cpp
)
target_link_libraries(cc1_ast PUBLIC cc1_basic)

cc1_add_module(cc1_lex
  lib/Lex/Lexer.cpp
  lib/Lex/Token.cpp
  lib/Lex/Preprocessor.cpp
  lib/Lex/PPExpression.cpp
)
target_link_libraries(cc1_lex PUBLIC cc1_basic)

cc1_add_module(cc1_parse
  lib/Parse/Parser.cpp
  lib/Parse/ParseDecl.cpp
  lib/Parse/ParseStmt.cpp
  lib/Parse/ParseExpr.cpp
)
target_link_libraries(cc1_parse PUBLIC cc1_ast cc1_lex)

cc1_add_module(cc1_sema
  lib/Sema/Sema.cpp
  lib/Sema/SemaDecl.cpp
  lib/Sema/SemaStmt.cpp
  lib/Sema/SemaExpr.cpp
  lib/Sema/SemaConversions.cpp
)
target_link_libraries(cc1_sema PUBLIC cc1_ast)

cc1_add_module(cc1_frontend
  lib/Frontend/CompilerInvocation.cpp
  lib/Frontend/CompilerInstance.cpp
)
target_link_libraries(cc1_frontend
  PUBLIC cc1_ast
  PRIVATE cc1_parse cc1_sema
)

add_executable(cc1
  tools/cc1/main.cpp
)

target_link_libraries(cc1
  PRIVATE
    cc1_frontend
    cc1_project_options
    cc1_warnings
)
option(CC1_ENABLE_MLIR "Enable MLIR code generation" OFF)

if(CC1_ENABLE_MLIR)
  enable_language(C)
  find_package(MLIR REQUIRED CONFIG)

  cc1_add_module(cc1_codegen
    lib/CodeGen/MLIRGen.cpp
    lib/CodeGen/MLIRGenTypes.cpp
    lib/CodeGen/MLIRGenExpr.cpp
    lib/CodeGen/MLIRGenBinary.cpp
    lib/CodeGen/MLIRGenControlFlow.cpp
    lib/CodeGen/MLIRGenFunction.cpp
    lib/CodeGen/MLIRGenStmt.cpp
    lib/CodeGen/MLIRGenMemory.cpp
  )

  target_include_directories(cc1_codegen SYSTEM PUBLIC
    ${LLVM_INCLUDE_DIRS}
    ${MLIR_INCLUDE_DIRS}
  )

  target_link_libraries(cc1_codegen PUBLIC
    cc1_ast
    MLIRIR
    MLIRArithDialect
    MLIRFuncDialect
    MLIRMemRefDialect
    MLIRControlFlowDialect
  )

  target_link_libraries(cc1_frontend PRIVATE cc1_codegen)
  target_compile_definitions(cc1_frontend PRIVATE CC1_ENABLE_MLIR=1)
endif()
if(BUILD_TESTING AND EXISTS ${PROJECT_SOURCE_DIR}/Tests/CMakeLists.txt)
  add_subdirectory(Tests)
endif()
