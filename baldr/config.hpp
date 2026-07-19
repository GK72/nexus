/**
 * Part of Baldr
 *
 * Configuration loading.
 *
 * @author      Gábor Krisztián Girhiny
 * @coauthor    Claude Sonnet 5 (Junie)
 * @date        2026-07-19
 */

#pragma once

#include <libnova/expected.hpp>
#include <libnova/error.hpp>
#include <libnova/yaml.hpp>

#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace baldr {

/**
 * @brief   Per-project settings, loaded from a `.baldr.yaml` (project-local)
 *          or `~/.baldr.yaml` (global) config file.
 */
struct config {
    std::string debugger = "gdb";
    std::vector<std::string> debugger_args = { "--args" };
    std::string build_type = "Debug";
    std::map<std::string, std::string> cmake_defines;
    std::map<std::string, std::string> env;
};

/**
 * @brief   Parse a `config` out of the YAML `doc`, keeping already
 *          initialized defaults for anything not present in `doc`.
 */
[[nodiscard]] auto parse(const nova::yaml& doc, config result) -> config;

/**
 * @brief   Parse `yaml_content` on top of `result`.
 *
 * @return  `result` merged with anything present in `yaml_content`, or an
 *          error if `yaml_content` is malformed.
 */
[[nodiscard]] auto
load(std::string_view yaml_content, config result) -> nova::expected<config, nova::error>;

/**
 * @brief   Parse a `.baldr.yaml` file at `path` on top of `result`, if it
 *          exists.
 *
 * @return  `result` unchanged if `path` doesn't exist, the merged config on
 *          success, or an error if the file exists but is malformed.
 */
[[nodiscard]] auto
load(const std::filesystem::path& path, baldr::config result) -> nova::expected<baldr::config, nova::error>;

/**
 * @brief   Load the config for `project_dir`.
 *
 * Falls back to global config (`$HOME/.baldr.yaml`) if there is no project
 * configuration.
 *
 * @param   project_dir     Project directory to look for a project-local
 *                          config in.
 *
 * @return  Loaded (or default) config, or an error if a found config
 *          file could not be parsed.
 */
[[nodiscard]] auto load(const std::filesystem::path& project_dir) -> nova::expected<config, nova::error>;

} // namespace baldr
