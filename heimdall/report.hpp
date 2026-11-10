/**
 * Part of Nexus Library
 *
 * @brief   The git views: the full report and the brief one.
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

#include <cstdint>
#include <string>
#include <vector>

namespace heimdall {

/// The counts behind the status marks.
struct marks_t {
    int conflicts = 0;
    int staged = 0;
    int modified = 0;
    int untracked = 0;
    int unpushed = 0;
    int behind = 0;
    int stashes = 0;
};

/**
 * @brief   The marks that are not zero, in priority order: `!1 +2 ~3 ?4 ↑5 ↓6 ≡7`.
 */
[[nodiscard]] auto badges(const style_t& style, const marks_t& marks) -> std::string;

/**
 * @brief   How fresh the remotes are: `fetched just now`, `oldest fetch 21h ago`
 *          or `never fetched`.
 */
[[nodiscard]] auto fetch_info(const std::vector<repo_t>& repos, bool fetched, std::int64_t now) -> std::string;

/**
 * @brief   The full report: a header with the totals, a block per repository
 *          that needs attention, and the legend.
 */
[[nodiscard]] auto render_report(const style_t& style, const std::vector<repo_t>& repos, bool fetched,
                                 std::int64_t now) -> std::string;

/**
 * @brief   One line per repository that needs attention: name, branch, marks, notes.
 */
[[nodiscard]] auto brief_rows(const style_t& style, const std::vector<repo_t>& repos) -> std::vector<std::string>;

/**
 * @brief   The brief report: a header with the totals and the brief rows.
 */
[[nodiscard]] auto render_brief(const style_t& style, const std::vector<repo_t>& repos, bool fetched,
                                std::int64_t now) -> std::string;

} // namespace heimdall
