/**
 * Part of Nexus Library
 *
 * @brief   Heimdall's configuration: which repositories it tracks.
 *
 * Read from `$XDG_CONFIG_HOME/nexus/heimdall.toml`, else
 * `~/.config/nexus/heimdall.toml`. A missing file means the defaults.
 *
 * @author      Gábor Krisztián Girhiny
 * @created     2026-11-10
 * @updated     2026-11-10
 * @status      experimental
 * @reviewed    none
 * SPDX-License-Identifier: Unlicense
 */

#pragma once

#include <libnova/dyn_value.hpp>
#include <libnova/error.hpp>
#include <libnova/expected.hpp>

#include <filesystem>
#include <string>
#include <vector>

namespace heimdall {

struct config_t {
    std::vector<std::string> roots { "~/repos" };   ///< Scanned one level deep
    std::vector<std::string> exclude_from_roots;    ///< Globs of repository names
    std::vector<std::string> repositories;          ///< Tracked one by one
};

/**
 * @brief   The configuration file's path.
 */
[[nodiscard]] auto config_path() -> std::filesystem::path;

/**
 * @brief   The configuration in `doc`, missing keys at their defaults.
 *
 * @returns an error if a key is not a list of strings.
 */
[[nodiscard]] auto parse_config(const nova::dyn_value& doc) -> nova::expected<config_t, nova::error>;

/**
 * @brief   The configuration at `path`; the defaults if there is no file.
 */
[[nodiscard]] auto load_config(const std::filesystem::path& path) -> nova::expected<config_t, nova::error>;

} // namespace heimdall
