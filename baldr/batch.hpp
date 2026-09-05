/**
 * Part of Baldr
 *
 * `baldr batch`: runs `.baldr.yaml`'s `batch:` stages/scenarios as plain
 * shell commands (see `baldr::config::batch`).
 *
 * @author      Gábor Krisztián Girhiny
 * @coauthor    Claude Sonnet 5 (Junie)
 * @date        2026-08-11
 */

#pragma once

#include <baldr/config.hpp>

#include <map>
#include <string>

namespace baldr {

/**
 * @brief   Runs `batch_config` stages/scenarios (see `baldr::config::batch`)
 *          inside `project_dir`.
 */
class batch_runner {
public:
    batch_runner(std::string project_dir, batch_config cfg);

    /**
     * @brief   Run the stage named `name` once.
     *
     * @param   params  Values for the stage's declared `parameters`, exposed
     *                  to its `cmd` as environment variables alongside
     *                  `batch.env`.
     *
     * @throws  nova::exception if `name` isn't a known stage, `params`
     *          doesn't cover every declared parameter (or names one that
     *          isn't declared), or the stage's command fails/is interrupted.
     */
    void run_stage(const std::string& name, const std::map<std::string, std::string>& params = {});

    /**
     * @brief   Run every step of the scenario named `name`, in order,
     *          stopping at the first failing stage.
     *
     * @throws  nova::exception if `name` isn't a known scenario, or (via
     *          `run_stage()`) a step's stage/parameters are invalid or its
     *          command fails/is interrupted.
     */
    void run_scenario(const std::string& name);

private:
    std::string m_project_dir;
    batch_config m_cfg;
};

} // namespace baldr
