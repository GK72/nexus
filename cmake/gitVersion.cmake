# gitVersion.cmake
#
# Provides `nexus_git_hash(<out_var>)`, resolving the short git commit hash of
# the current checkout (falling back to "unknown" when git or the repository
# itself isn't available, e.g. a source tarball build).
#
# This module is intentionally self-contained (no dependency on other Nexus
# CMake code) since it is expected to be migrated out into a standalone,
# reusable CMake module in the future.

function(nexus_git_hash out_var)
    find_package(Git QUIET)

    set(hash "unknown")
    if(Git_FOUND)
        execute_process(
            COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            OUTPUT_VARIABLE result
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
            RESULT_VARIABLE exit_code
        )
        if(exit_code EQUAL 0 AND NOT result STREQUAL "")
            set(hash "${result}")
        endif()
    endif()

    set(${out_var} "${hash}" PARENT_SCOPE)
endfunction()
