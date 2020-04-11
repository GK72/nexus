add_library(gmath gmath.cpp gmath.h)
add_library(bigint bigint.cpp bigint.h)
add_library(data datatable.cpp datatable.h)
add_library(date datetime.cpp datetime.h)
add_library(io io.cpp io.h)
add_library(arg arg.cpp arg.h)
add_library(utility utility.cpp utility.h)

target_link_libraries(data PRIVATE utility)
target_link_libraries(data PRIVATE io)

include_directories(${CMAKE_HOME_DIRECTORY}/include)
