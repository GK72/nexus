cmake_minimum_required(VERSION 3.24)

if(CMAKE_BINARY_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    message(FATAL_ERROR "Building in-source is not supported! Create a build dir and remove ${CMAKE_SOURCE_DIR}/CMakeCache.txt")
endif()

# include(version.cmake)
# set(NOVA_VERSION_FILE "${CMAKE_CURRENT_LIST_DIR}/libnova/details/version.hpp")
# nova_extract_version("${NOVA_VERSION_FILE}")

set(BALDR_VERSION "0.0.1")
project("baldr-cpp" VERSION ${BALDR_VERSION} LANGUAGES CXX)
message(STATUS "Baldr version ${BALDR_VERSION}")

include(dependencies.cmake)

include(novaCompilerWarnings)
include(novaCompilerSettings)

include(CTest)

add_subdirectory(baldr)

include(install.cmake)
