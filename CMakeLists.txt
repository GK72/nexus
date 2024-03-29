cmake_minimum_required(VERSION 3.23)

if (CMAKE_BINARY_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    message(FATAL_ERROR "Building in-source is not supported! Create a build dir and remove ${CMAKE_SOURCE_DIR}/CMakeCache.txt")
endif()

project("Nexus" VERSION 0.2.0)

find_package(cppfront REQUIRED)

add_library(project_warnings INTERFACE)
include(cmake/compiler-warnings.cmake)
set_project_warnings(project_warnings)

include(cmake/settings.cmake)

add_subdirectory(deps)
add_subdirectory(include)
add_subdirectory(src)

enable_testing()
add_subdirectory(unit-tests)
