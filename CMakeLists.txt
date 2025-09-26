cmake_minimum_required(VERSION 4.1)
project(NES)

file(GLOB_RECURSE SOURCES source/*.cpp)
add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE source)

target_precompile_headers(${PROJECT_NAME}
   PRIVATE source/pch.hpp)

set_target_properties(${PROJECT_NAME} PROPERTIES
   CXX_STANDARD 23
   CXX_STANDARD_REQUIRED ON)

target_compile_options(${PROJECT_NAME}
   PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic -Werror>
   PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>)