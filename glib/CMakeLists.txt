add_library (gmath gmath.cpp)
add_library (utility utility.cpp)

target_include_directories (gmath PUBLIC h)
target_include_directories (utility PUBLIC h)