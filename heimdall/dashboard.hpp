/**
 * Part of Nexus Library
 *
 * @brief   The dashboard: Heimdall's default view, the welcome screen of a shell.
 *
 * @author      Gábor Krisztián Girhiny
 * @created     2026-11-10
 * @updated     2026-11-10
 * @status      experimental
 * @reviewed    none
 * SPDX-License-Identifier: Unlicense
 */

#pragma once

#include "heimdall/git.hpp"
#include "heimdall/style.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace heimdall {

struct disk_t {
    std::string label;      ///< `/`, `~` or the mount point
    double ratio = 0.0;     ///< Used of the total
};

/// What the system section shows.
struct system_t {
    std::string host;
    std::string os;
    std::string kernel;
    std::int64_t uptime = 0;    ///< Seconds
    std::vector<disk_t> disks;
};

/**
 * @brief   The facts of this machine; the disks are `/`, the home directory
 *          and `/mnt/data`, each filesystem once.
 */
[[nodiscard]] auto read_system() -> system_t;

struct dashboard_input {
    std::size_t width = 96;                         ///< Columns, without the margin
    std::chrono::system_clock::time_point now;
    system_t system;
    std::vector<repo_t> repos;
    bool fetched = false;
    std::size_t repo_limit = 8;
    std::chrono::milliseconds elapsed {};
};

[[nodiscard]] auto render_dashboard(const style_t& style, const dashboard_input& input) -> std::string;

} // namespace heimdall
