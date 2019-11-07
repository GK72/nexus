add_executable(simpleTest tester.cpp)
add_test(Test1 simpleTest)

target_include_directories(simpleTest PUBLIC ../../Catch2/include)
