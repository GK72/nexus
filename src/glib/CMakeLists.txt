add_library(date        datetime.cpp    datetime.h)
add_library(io          io.cpp          io.h)
add_library(arg         arg.cpp         arg.h)
add_library(utility     utility.cpp     utility.h)

include_directories(${CMAKE_HOME_DIRECTORY}/include)
