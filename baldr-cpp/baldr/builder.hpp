#pragma once

#include <baldr/config.hpp>
#include <string>

namespace baldr {

/**
 * @brief Orchestrates CMake build operations.
 */
class Builder {
public:
    /**
     * @brief Construct a new Builder.
     * 
     * @param config The configuration to use.
     */
    explicit Builder(const Config& config);

    /**
     * @brief Run CMake configuration.
     * 
     * @param project_path Path to the project root.
     * @param build_dir Path to the build directory.
     * @returns Exit code of the cmake process.
     */
    auto configure(const std::string& project_path, const std::string& build_dir) -> int;

    /**
     * @brief Run CMake build.
     * 
     * @param build_dir Path to the build directory.
     * @param target The CMake target to build.
     * @returns Exit code of the cmake process.
     */
    auto build(const std::string& build_dir, const std::string& target) -> int;

    /**
     * @brief Run a debugger on a project executable.
     * 
     * @param build_dir Path to the build directory.
     * @param exe_args Arguments to forward to the executable.
     * @returns Exit code of the debugger.
     */
    auto debug(const std::string& build_dir, const std::vector<std::string>& exe_args) -> int;

private:
    const Config& m_config;
};

} // namespace baldr
