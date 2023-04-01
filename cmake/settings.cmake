find_program(CCACHE ccache)
if(CCACHE)
    message("Using ccache")
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE})
else()
    message("Ccache cannot be found")
endif()

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 23)

if(COVERAGE)
    set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-g -O0 -fno-inline -fprofile-arcs -ftest-coverage")
endif()

if(SANITIZERS)
    if(SANITIZERS STREQUAL asan)
        set(SANITIZER_LIST -fsanitize=address -fsanitize=leak -fsanitize=undefined)
    endif()
    if(SANITIZERS STREQUAL tsan)
        set(SANITIZER_LIST -fsanitize=thread)
    endif()
endif()

function(code_analysis TARGET VISIBILITY)
    target_link_libraries(${TARGET} ${VISIBILITY} project_warnings)

    if(${SANITIZERS} MATCHES "[at]san")
        target_compile_options(${TARGET} ${VISIBILITY} ${SANITIZER_LIST})
        target_link_options(${TARGET} ${VISIBILITY} ${SANITIZER_LIST})
        message("Code analysis is turned on for ${TARGET} with ${SANITIZER_LIST}")
    else()
        message("Code analysis is turned on for ${TARGET}")
    endif()
endfunction()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_EXPERIMENTAL_CXX_MODULE_CMAKE_API "2182bf5c-ef0d-489a-91da-49dbc3090d2a")
set(CMAKE_EXPERIMENTAL_CXX_MODULE_DYNDEP 1)
