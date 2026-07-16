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

#include <string>

namespace baldr {

/**
 * @brief   Wraps a project's build invocation via `baldr::command`.
 */
class builder {
public:
    explicit builder(std::string project_dir = ".", std::string build_type = "debug");

    /**
     * @brief   Run the build command inside `project_dir`, streaming its
     *          combined stdout/stderr output via `nova::log::info`.
     *
     * @throws  nova::exception if the build command exits with a non-zero
     *          code.
     */
    void build() const;

    /**
     * @brief   Run `target`, attached to the caller's own TTY.
     *
     * @param   target  Executable name (relative to `project_dir`) to run.
     *
     * @throws  nova::exception if `target` exits with a non-zero code.
     */
    void run(const std::string& target) const;

private:
    std::string m_project_dir;
    std::string m_build_type;
};

} // namespace baldr
