install(TARGETS nexus project_warnings EXPORT nexusTargets)
install(DIRECTORY "${CMAKE_HOME_DIRECTORY}/include/" DESTINATION "include")
export(EXPORT nexusTargets FILE "${CMAKE_CURRENT_BINARY_DIR}/nexusTargets.cmake")

install(
    EXPORT nexusTargets
    FILE nexusTargets.cmake
    DESTINATION "lib/cmake/nexus"
)

set(INTERNAL_CMAKE_FILES "(conan.cmake|config.cmake.in|install.cmake)")
install(
    DIRECTORY "${CMAKE_HOME_DIRECTORY}/cmake/"
    DESTINATION "lib/cmake/nexus"
    REGEX ${INTERNAL_CMAKE_FILES} EXCLUDE
)

include(CMakePackageConfigHelpers)

# Used by `find_package`
configure_package_config_file(
    "${CMAKE_HOME_DIRECTORY}/cmake/config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/nexusConfig.cmake"
    INSTALL_DESTINATION "lib/cmake"
    NO_SET_AND_CHECK_MACRO
    NO_CHECK_REQUIRED_COMPONENTS_MACRO
)

# Used by `find_package`
write_basic_package_version_file(
    "${CMAKE_HOME_DIRECTORY}/nexusConfigVersion.cmake"
    VERSION "${Nexus_VERSION_MAJOR}.${Nexus_VERSION_MINOR}"
    COMPATIBILITY AnyNewerVersion
)
