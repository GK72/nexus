include("${CMAKE_CURRENT_LIST_DIR}/compilerWarnings.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/settings.cmake")

add_library(project_warnings INTERFACE)
set_project_warnings(project_warnings)
