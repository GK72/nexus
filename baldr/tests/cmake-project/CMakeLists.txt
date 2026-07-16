cmake_minimum_required(VERSION 3.20)
project(baldr-cmake-testproject CXX)

# Minimal CMake-based project, used to manually exercise `baldr build`/`baldr
# run` against a CMake build system.

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(hello main.cpp)
