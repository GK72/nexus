/**
 * Part of Nexus Library
 *
 * @brief   Heimdall's configuration: which repositories it tracks, where norn
 *          and Claude's data are, and what the dashboard shows.
 *
 * Read from `$XDG_CONFIG_HOME/nexus/heimdall.toml`, else
 * `~/.config/nexus/heimdall.toml`. A missing file means the defaults.
 *
 * @author      Gábor Krisztián Girhiny
 * @created     2026-11-10
 * @updated     2026-11-14
 * @status      experimental
 * @reviewed    none
 * SPDX-License-Identifier: Unlicense
 */

#pragma once

#include <libnova/dyn_value.hpp>
#include <libnova/error.hpp>
#include <libnova/expected.hpp>

#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace heimdall {

struct config_t {
    std::vector<std::string> roots { "~/repos" };   ///< Scanned one level deep
    std::vector<std::string> exclude_from_roots;    ///< Globs of repository names
    std::vector<std::string> repositories;          ///< Tracked one by one
    std::vector<std::string> norn { "norn.py" };    ///< The norn client, with its arguments
    std::map<std::string, std::string> projects;    ///< norn project to repository name or path
    std::string claude { "~/.claude/projects" };    ///< Claude Code's projects: transcripts and memory
    std::vector<std::string> memory;                ///< Memory directories; empty: every project's
    std::vector<std::string> dashboard { "system", "repos", "tasks", "memory" };  ///< Sections, in order
};

/// The sections the dashboard knows.
[[nodiscard]] auto dashboard_sections() -> const std::vector<std::string>&;

/**
 * @brief   The configuration file's path.
 */
[[nodiscard]] auto config_path() -> std::filesystem::path;

/**
 * @brief   The configuration in `doc`, missing keys at their defaults.
 *
 * @returns an error if a key has the wrong type, or the dashboard names a
 *          section it does not know.
 */
[[nodiscard]] auto parse_config(const nova::dyn_value& doc) -> nova::expected<config_t, nova::error>;

/**
 * @brief   The configuration at `path`; the defaults if there is no file.
 */
[[nodiscard]] auto load_config(const std::filesystem::path& path) -> nova::expected<config_t, nova::error>;

} // namespace heimdall
