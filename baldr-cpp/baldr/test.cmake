function(add_test_target MODULE_NAME)
    set(target "test-${MODULE_NAME}")
    file(GLOB_RECURSE TEST_SOURCES "*.test.cpp")
    add_executable("${target}" ${TEST_SOURCES} "${CMAKE_CURRENT_LIST_DIR}/tty.cpp" "${CMAKE_CURRENT_LIST_DIR}/progress.cpp")

    target_include_directories(${target} PRIVATE "${BALDR_PROJECT_ROOT}")
    target_link_libraries(${target} PRIVATE
        nova::nova

        GTest::gtest_main
        GTest::gtest
        GTest::gmock
    )

    gtest_discover_tests(
        ${target}
        DISCOVERY_MODE PRE_TEST
    )
endfunction()
