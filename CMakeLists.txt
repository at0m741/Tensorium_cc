cmake_minimum_required(VERSION 3.22)

project(cc1 VERSION 0.1.0 LANGUAGES CXX)

include(CTest)

option(CC1_ENABLE_WARNINGS "Enable extra compiler warnings" ON)

set(CMAKE_CXX_EXTENSIONS OFF)

set(CC1_INCLUDE_DIRS
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/src
)

add_library(cc1_project_options INTERFACE)
target_compile_features(cc1_project_options INTERFACE cxx_std_17)

add_library(cc1_warnings INTERFACE)
if(CC1_ENABLE_WARNINGS)
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(cc1_warnings INTERFACE -O3 -Wall -Wextra -Wpedantic)
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

cc1_add_module(cc1_lexer
  src/lexer/Lexer.cpp
  src/lexer/Token.cpp
)

cc1_add_module(cc1_parser
  src/parser/Parser.cpp
  src/parser/Pratt.cpp
)
target_link_libraries(cc1_parser PUBLIC cc1_lexer)

cc1_add_module(cc1_sema
  src/sema/Sema.cpp
)
target_link_libraries(cc1_sema PUBLIC cc1_parser)

cc1_add_module(cc1_codegen
  src/codegen/IRGen.cpp
)
target_link_libraries(cc1_codegen PUBLIC cc1_sema)

add_library(cc1_frontend INTERFACE)
target_link_libraries(cc1_frontend INTERFACE cc1_codegen)

add_executable(cc1
  src/main.cpp
)

target_link_libraries(cc1
  PRIVATE
    cc1_frontend
    cc1_project_options
    cc1_warnings
)

if(BUILD_TESTING AND EXISTS ${PROJECT_SOURCE_DIR}/Tests/CMakeLists.txt)
  add_subdirectory(Tests)
endif()
