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
   PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>)

find_package(SDL3 CONFIG REQUIRED COMPONENTS SDL3-static)
find_package(imgui CONFIG REQUIRED COMPONENTS imgui-static)
target_link_libraries(${PROJECT_NAME}
   PRIVATE SDL3::SDL3-static
   PRIVATE imgui::imgui)

if(NOT EMSCRIPTEN)
   find_package(nfd CONFIG REQUIRED COMPONENTS nfd-static)
   target_link_libraries(${PROJECT_NAME}
      PRIVATE nfd::nfd)

   target_precompile_headers(${PROJECT_NAME}
      PRIVATE source/pch.hpp)
else()
   set(EMSCRIPTEN_WEB_DIRECTORY ${CMAKE_SOURCE_DIR}/web)
   set(EMSCRIPTEN_SHELL_FILE ${EMSCRIPTEN_WEB_DIRECTORY}/shell.html)

   set_target_properties(${PROJECT_NAME} PROPERTIES
      SUFFIX ".html"
      LINK_DEPENDS ${EMSCRIPTEN_SHELL_FILE})

   target_compile_options(${PROJECT_NAME}
      PRIVATE -pthread)

   target_link_libraries(${PROJECT_NAME}
      PRIVATE -sASSERTIONS
      PRIVATE -sUSE_PTHREADS
      PRIVATE -sPTHREAD_POOL_SIZE=2
      PRIVATE -sALLOW_MEMORY_GROWTH
      PRIVATE --shell-file=${EMSCRIPTEN_SHELL_FILE})

   add_custom_target(${PROJECT_NAME}_host_locally
      DEPENDS ${PROJECT_NAME}
      USES_TERMINAL
      COMMAND node server.js -d ${CMAKE_BINARY_DIR} -f ${PROJECT_NAME}.html
      WORKING_DIRECTORY ${EMSCRIPTEN_WEB_DIRECTORY})
endif()