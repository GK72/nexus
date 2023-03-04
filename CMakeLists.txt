cmake_minimum_required(VERSION 3.12)

if (CMAKE_BINARY_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    message(FATAL_ERROR "Building in-source is not supported! Create a build dir and remove ${CMAKE_SOURCE_DIR}/CMakeCache.txt")
endif()

project("NexusLib")

include(cmake/conan.cmake)
run_conan()

add_library(project_warnings INTERFACE)
include(cmake/compilerWarnings.cmake)
set_project_warnings(project_warnings)

include(cmake/settings.cmake)

add_subdirectory(src)

if(NOT PACKAGING)
    add_subdirectory(test)
endif()

enable_testing()
