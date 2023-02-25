cmake_minimum_required(VERSION 3.12)

if (CMAKE_BINARY_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    message(FATAL_ERROR "Building in-source is not supported! Create a build dir and remove ${CMAKE_SOURCE_DIR}/CMakeCache.txt")
endif()

project("example")

set(CMAKE_CXX_STANDARD 23)

include(conan.cmake)
run_conan()

add_subdirectory(src)
