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

#include <baldr/config.hpp>

#include <filesystem>
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
    /**
     * @brief   Construct a `builder` for `project_dir`, sourcing build
     *          type, CMake defines/env and debugger settings from `cfg`.
     *
     * @param   project_dir     Project directory to build/run in.
     * @param   cfg             Resolved configuration (already merged with
     *                          any CLI overrides by the caller).
     */
    builder(
        std::string project_dir = ".",
        config cfg = {}
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
     * @param   debug           If `true`, launch `target` under the
     *                          configured debugger (`m_debugger`/
     *                          `m_debugger_args`) instead of running it
     *                          directly.
     *
     * @throws  nova::exception if `target` exits with a non-zero code.
     */
    void run(const std::string& target, const std::vector<std::string>& forwarded_args = {}, bool debug = false);

private:
    std::string m_project_dir;
    std::string m_build_type;
    std::map<std::string, std::string> m_cmake_defines;
    std::map<std::string, std::string> m_cmake_env;
    std::string m_debugger;
    std::vector<std::string> m_debugger_args;
    project_type m_project_type { project_type::make };


    [[nodiscard]] auto
    discover_project_type(bool clean_build) -> std::vector<std::string>;

    [[nodiscard]] auto
    resolve_executable(const std::string& target) const -> std::string;

    [[nodiscard]] auto
    handle_makefile_project(bool clean_build) const -> std::vector<std::string>;

    void configure_cmake(
        const std::filesystem::path& build_dir,
        const std::string& build_dir_rel,
        const std::string& resolved_defines
    ) const;

    [[nodiscard]] auto build_argv(
        const std::string& exe_path,
        const std::vector<std::string>& forwarded_args,
        bool debug
    ) const -> std::vector<std::string>;
};

} // namespace baldr
