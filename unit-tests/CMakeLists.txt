add_executable(test-nexus
    ph.cc
)

target_link_libraries(test-nexus PRIVATE
    nexus
    gtest_main
    gtest
)

add_test(test-nexus test-nexus)
