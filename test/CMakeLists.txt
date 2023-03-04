add_executable(test_nexus
    detector.cc
    flat_map.cc
    nexus.cc
    units.cc
)

target_link_libraries(test_nexus
    ${CONAN_LIBS}
    project_warnings
    nexus
)

target_include_directories(test_nexus PRIVATE ${CMAKE_HOME_DIRECTORY})

include(CTest)
include(Catch)
catch_discover_tests(test_nexus)
