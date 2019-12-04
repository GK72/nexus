cmake_minimum_required(VERSION 3.10)

if (CMAKE_BINARY_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  message(FATAL_ERROR "Building in-source is not supported! Create a build dir and remove ${CMAKE_SOURCE_DIR}/CMakeCache.txt")
endif()

project("chai")

set(CMAKE_CXX_STANDARD 17)
set(LINKER_FLAGS "${LINKER_FLAGS} -pthread")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${LINKER_FLAGS}")

add_executable(chai main.cpp)
add_library(functions SHARED functions.cpp)
target_link_libraries(chai functions dl)
