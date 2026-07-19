/**
 * Part of Baldr
 *
 * The builder. Contains the business logic.
 *
 * @author      Gábor Krisztián Girhiny
 * @coauthor    Claude Sonnet 5 (Junie)
 * @date        2026-07-12
 */

#pragma once

#include <map>
#include <string>
#include <vector>

namespace baldr {

enum class project_type {
    make,
    cmake
};

/**
 * @brief   Wraps a project's build invocation via `baldr::command`.
 */
class builder {
public:
    builder(
        std::string project_dir = ".",
        std::string build_type = "Debug",
        std::map<std::string, std::string> cmake_defines = {},
        std::map<std::string, std::string> cmake_env = {}
    );

    /**
     * @brief   Run the build command inside `project_dir`, streaming its
     *          combined stdout/stderr output via `nova::log::info`.
     *
     * @param   clean_build     If `true`, wipe the resolved build directory
     *                          (CMake) or run `make clean` (Makefile, if a
     *                          `clean` target exists) before building.
     *
     * @throws  nova::exception if the build command exits with a non-zero
     *          code.
     */
    void build(bool clean_build = false);

    /**
     * @brief   Run `target`, attached to the caller's own TTY.
     *
     * @param   target          Executable name (relative to `project_dir`)
     *                          to run.
     * @param   forwarded_args  Extra arguments appended after `target`'s own
     *                          path, forwarded verbatim to its argv (e.g.
     *                          everything following a literal `--` on
     *                          baldr's own command line).
     *
     * @throws  nova::exception if `target` exits with a non-zero code.
     */
    void run(const std::string& target, const std::vector<std::string>& forwarded_args = {});

private:
    std::string m_project_dir;
    std::string m_build_type;
    std::map<std::string, std::string> m_cmake_defines;
    std::map<std::string, std::string> m_cmake_env;
    project_type m_project_type { project_type::make };


    [[nodiscard]] auto
    discover_project_type(bool clean_build) -> std::vector<std::string>;
};

} // namespace baldr
