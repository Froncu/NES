cmake_minimum_required(VERSION 4.1)
project(frones)

file(GLOB_RECURSE SOURCES source/*.cpp)
add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME}
   PRIVATE source)

set_target_properties(${PROJECT_NAME} PROPERTIES
   CXX_STANDARD 23
   CXX_STANDARD_REQUIRED ON)

target_compile_options(${PROJECT_NAME}
   PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic -Werror>
   PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
   PRIVATE $<$<BOOL:${EMSCRIPTEN}>:-pthread>)

find_package(SDL3 CONFIG REQUIRED COMPONENTS SDL3-static)
find_package(imgui CONFIG REQUIRED COMPONENTS imgui-static)

target_link_libraries(${PROJECT_NAME}
   PRIVATE SDL3::SDL3-static
   PRIVATE imgui::imgui
   PRIVATE $<$<BOOL:${EMSCRIPTEN}>:-sASSERTIONS -sUSE_PTHREADS -sPTHREAD_POOL_SIZE=2 -sALLOW_MEMORY_GROWTH
   --shell-file=${CMAKE_SOURCE_DIR}/web/shell.html>)

target_precompile_headers(${PROJECT_NAME}
   PRIVATE $<$<NOT:$<BOOL:${EMSCRIPTEN}>>:${CMAKE_SOURCE_DIR}/source/pch.hpp>)

if(NOT EMSCRIPTEN)
   find_package(nfd CONFIG REQUIRED COMPONENTS nfd-static)
   target_link_libraries(${PROJECT_NAME}
      PRIVATE nfd::nfd)
else()
   set_target_properties(${PROJECT_NAME} PROPERTIES
      SUFFIX ".html")

   add_custom_target(${PROJECT_NAME}_host_locally
      DEPENDS ${PROJECT_NAME}
      USES_TERMINAL
      COMMAND node ${CMAKE_SOURCE_DIR}/web/server.js -d ${CMAKE_BINARY_DIR} -p 8000 &
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/web)
endif()