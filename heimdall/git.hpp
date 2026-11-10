/**
 * Part of Nexus Library
 *
 * @brief   The state of the tracked Git repositories: what needs attention.
 *
 * A survey finds the repositories (see `config_t`), optionally fetches them,
 * and reads each one's worktrees, branches and stashes, in parallel. The
 * parsers of Git's output are pure functions of its text.
 *
 * @author      Gábor Krisztián Girhiny
 * @created     2026-11-10
 * @updated     2026-11-10
 * @status      experimental
 * @reviewed    none
 * SPDX-License-Identifier: Unlicense
 */

#pragma once

#include "heimdall/config.hpp"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace heimdall {

/// Changes in a worktree, and how its branch stands to its upstream.
struct status_t {
    int staged = 0;
    int modified = 0;
    int untracked = 0;
    int conflicts = 0;
    int ahead = 0;
    int behind = 0;
    std::optional<std::string> upstream;

    [[nodiscard]] auto dirty() const -> bool { return staged + modified + untracked + conflicts > 0; }
};

struct worktree_t {
    std::string path;
    std::optional<std::string> branch;      ///< Empty when detached
    std::string head;                       ///< Abbreviated commit
    bool main = false;                      ///< The repository's main worktree
    bool locked = false;
    status_t status;
};

struct branch_t {
    std::string name;
    std::string head;
    std::string subject;
    std::int64_t committed = 0;             ///< Unix time of the last commit
    std::optional<std::string> upstream;
    std::optional<std::string> worktree;    ///< Where it is checked out
    bool gone = false;                      ///< Its upstream was deleted
    int ahead = 0;
    int behind = 0;
    int unpushed = 0;                       ///< Commits on no remote

    [[nodiscard]] auto notable() const -> bool { return unpushed > 0 or behind > 0 or gone; }
};

struct stash_t {
    std::string ref;
    std::int64_t created = 0;
    std::string message;
};

struct repo_t {
    std::string name;
    std::string path;
    std::vector<std::string> remotes;
    std::optional<std::int64_t> last_fetch; ///< Unix time of FETCH_HEAD
    std::optional<std::string> fetch_error;
    std::optional<std::string> error;
    std::vector<worktree_t> worktrees;
    std::vector<branch_t> branches;
    std::vector<stash_t> stashes;
    int unpushed = 0;                       ///< Distinct commits of all branches on no remote

    [[nodiscard]] auto local_only() const -> bool { return remotes.empty(); }
    [[nodiscard]] auto dirty() const -> bool;
    [[nodiscard]] auto behind() const -> int;
    [[nodiscard]] auto gone() const -> int;
    [[nodiscard]] auto attention() const -> bool;
};

struct survey_options {
    bool fetch = false;
    int jobs = 8;
    std::vector<std::string> repositories;          ///< Tracked besides the configured ones
    std::vector<std::string> exclude_from_roots;    ///< Besides the configured globs
};

/**
 * @brief   The tracked repositories, by name.
 */
[[nodiscard]] auto survey(const config_t& config, const survey_options& options) -> std::vector<repo_t>;

/**
 * @brief   The main worktrees of the repositories to track, each once.
 *
 * Every repository directly under a root whose name matches no glob of
 * `exclude`, a root that is a repository itself, and the `repositories`.
 * Linked worktrees and submodules (a `.git` file) are never found on their own.
 */
[[nodiscard]] auto discover(const std::vector<std::string>& roots, const std::vector<std::string>& exclude,
                            const std::vector<std::string>& repositories) -> std::vector<std::filesystem::path>;

/**
 * @brief   The worktrees of `git worktree list --porcelain`, without the bare
 *          and the prunable ones.
 */
[[nodiscard]] auto parse_worktrees(std::string_view text) -> std::vector<worktree_t>;

/**
 * @brief   The status of `git status --porcelain=v2 --branch`.
 */
[[nodiscard]] auto parse_status(std::string_view text) -> status_t;

/**
 * @brief   The branches of `git for-each-ref` in `BranchFormat`.
 */
[[nodiscard]] auto parse_branches(std::string_view text) -> std::vector<branch_t>;

/**
 * @brief   The stashes of `git stash list` in `StashFormat`.
 */
[[nodiscard]] auto parse_stashes(std::string_view text) -> std::vector<stash_t>;

/**
 * @brief   The telling line of a failed fetch: the cause, such as ssh's
 *          "Permission denied", not the advice that follows it.
 */
[[nodiscard]] auto fetch_reason(std::string_view stderr_text) -> std::string;

inline constexpr std::string_view BranchFormat =
    "%(refname:short)%00%(objectname:short)%00%(committerdate:unix)%00%(upstream:short)%00"
    "%(upstream:track,nobracket)%00%(worktreepath)%00%(subject)";

inline constexpr std::string_view StashFormat = "%gd%x00%ct%x00%gs";

} // namespace heimdall
