# C++26 with reflection and contracts (GCC 16): Nova's experimental feature set.
set(NOVA_EXPERIMENTAL_FEATURE_SET ON)
set(SANITIZERS "none" CACHE STRING "Sanitizers: none, asan or tsan")
set_property(CACHE SANITIZERS PROPERTY STRINGS none asan tsan)

include(novaCompilerWarnings)
include(novaCompilerSettings)
include(novaBuildInfo)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
# No C++20 modules are used; skip the dependency scanning CMake enables for C++26.
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

# All executables (the program, the tests) in one place: `<build>/bin`.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# Warnings (as errors) and sanitizers for every target defined after this line.
add_library(code-analysis INTERFACE)
code_analysis(code-analysis INTERFACE)
link_libraries(code-analysis)
