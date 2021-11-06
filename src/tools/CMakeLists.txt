set(IMGUI_DIR ${CMAKE_HOME_DIRECTORY}/.deps/imgui)

add_executable(gui gui.cpp)

target_include_directories(gui PRIVATE
    ${CMAKE_HOME_DIRECTORY}/src/ncx
    ${CMAKE_HOME_DIRECTORY}/.deps/ChaiScript/include/chaiscript
    ${CMAKE_HOME_DIRECTORY}/.deps/spdlog/include
    ${CMAKE_HOME_DIRECTORY}/.deps/spdlog/include/spdlog
    ${IMGUI_DIR}
)

add_library(imgui
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
)

target_include_directories(imgui PRIVATE ${IMGUI_DIR})

target_link_libraries(gui PRIVATE
    imgui
    glfw
    GL
    ${CMAKE_DL_LIBS}
)

add_executable(sandbox sandbox.cpp)

target_link_libraries (sandbox PRIVATE
    utility
    io
    arg
    ui
)

target_include_directories(sandbox PRIVATE
    ${CMAKE_HOME_DIRECTORY}/include
    ../ncx
    ../ui
)

add_executable (bench bench.cpp)
target_link_libraries(bench benchmark::benchmark)

add_executable (worklog worklog.cpp)

target_link_libraries (worklog PRIVATE
    utility
    arg
    date
)

target_include_directories(worklog PRIVATE
    ${CMAKE_HOME_DIRECTORY}/include
    ../ncx
)

if (UNIX)
    target_link_libraries(sandbox PRIVATE pthread)
    target_link_libraries(worklog PRIVATE pthread)
    target_link_libraries(gui PRIVATE pthread)
endif()
