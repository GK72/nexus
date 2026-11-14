/**
 * Part of Nexus Library
 *
 * @brief   The tasks of norn: the plan, its dependencies, and its views.
 *
 * @author      Gábor Krisztián Girhiny
 * @created     2026-11-14
 * @updated     2026-11-14
 * @status      experimental
 * @reviewed    none
 * SPDX-License-Identifier: Unlicense
 */

#pragma once

#include "heimdall/claude.hpp"
#include "heimdall/style.hpp"

#include <libnova/dyn_value.hpp>
#include <libnova/error.hpp>
#include <libnova/expected.hpp>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace heimdall {

/// A task of norn, as its API has it.
struct task_t {
    std::string id;                         ///< `<project>/<area>/<name>`
    std::string project;
    std::string kind;                       ///< feature, debt, bug, ...
    std::string title;
    std::string refs;
    std::string status;                     ///< open, done or dropped
    std::string state;                      ///< proposed, active, ready, blocked, done or dropped
    std::string added;                      ///< YYYY-MM-DD
    std::optional<std::string> closed;
    std::string note;
    double original = 0.0;                  ///< Hours, as first estimated
    double current = 0.0;                   ///< Hours, as estimated now
    double spent = 0.0;                     ///< Hours
    int moves = 0;                          ///< Times it was rescheduled
    int priority = 2;                       ///< 1 (high) to 3 (low)
    std::string source = "author";          ///< author, or claude when Claude added it on its own
    bool approved = true;                   ///< False while a task Claude added waits for approval
    std::int64_t tokens = 0;                ///< Tokens estimated; 0 if not
    bool shelved = false;                   ///< Its project is shelved
    std::int64_t tokens_spent = 0;          ///< Tokens of the sessions linked to it

    [[nodiscard]] auto open() const -> bool { return status == "open"; }
    [[nodiscard]] auto proposed() const -> bool { return open() and not approved; }
    [[nodiscard]] auto by_claude() const -> bool { return source == "claude"; }
    [[nodiscard]] auto left() const -> double { return current > spent ? current - spent : 0.0; }
    /// The id without the project: `gpu/device` of `freyja/gpu/device`.
    [[nodiscard]] auto name() const -> std::string_view;
};

/// `task` waits on `needs`.
struct dep_t {
    std::string task;
    std::string needs;
};

/// A Claude session that worked on `task`.
struct session_link_t {
    std::string task;
    std::string session;
};

/// The tasks, their dependencies and the Claude sessions linked to them.
struct plan_t {
    std::vector<task_t> tasks;              ///< By id
    std::vector<dep_t> deps;
    std::vector<session_link_t> sessions;

    /// The ids of the linked sessions.
    [[nodiscard]] auto session_ids() const -> std::set<std::string>;

    [[nodiscard]] auto find(std::string_view id) const -> const task_t*;
    /// The tasks `id` waits on, by id.
    [[nodiscard]] auto needs(std::string_view id) const -> std::vector<const task_t*>;
    /// The tasks that wait on `id`, by id.
    [[nodiscard]] auto needed_by(std::string_view id) const -> std::vector<const task_t*>;
};

/**
 * @brief   The plan in `doc`, the answer of norn's `graph --format json`:
 *          `{"tasks": [...], "deps": [{"task", "needs"}, ...]}`.
 */
[[nodiscard]] auto parse_plan(const nova::dyn_value& doc) -> nova::expected<plan_t, nova::error>;

/**
 * @brief   Sets the tokens each task spent from the `usage` of the sessions
 *          linked to it; a session linked to several tasks is split evenly.
 */
void attribute_tokens(plan_t& plan, const std::map<std::string, usage_t>& usage);

/**
 * @brief   The plan from the norn client `command`, run with
 *          `graph --format json`.
 */
[[nodiscard]] auto load_plan(const std::vector<std::string>& command) -> nova::expected<plan_t, nova::error>;

/**
 * @brief   The repository of each project: the one named in `projects` (a name
 *          or a path), else the one named like the project; `repos` are the
 *          tracked repositories. A project without one is left out.
 */
[[nodiscard]] auto project_repos(const plan_t& plan, const std::vector<std::filesystem::path>& repos,
                                 const std::map<std::string, std::string>& projects)
    -> std::map<std::string, std::string>;

struct tasks_view {
    std::size_t width = 96;                         ///< Columns, without the margin
    std::map<std::string, std::string> repos;       ///< Project to repository path
    bool shelved = false;                           ///< Show shelved projects too
};

/**
 * @brief   The open tasks whose id starts with `prefix` (all for an empty
 *          one): first the ones waiting for approval, then per project a tree
 *          from the tasks that can start to the ones they unblock, by
 *          priority. Shelved projects are only counted, unless `view.shelved`
 *          or `prefix` is in one.
 */
[[nodiscard]] auto render_tasks(const style_t& style, const plan_t& plan, std::string_view prefix,
                                const tasks_view& view) -> std::string;

/**
 * @brief   One task in full: its plan, what it waits on and what waits on it,
 *          its refs and note.
 */
[[nodiscard]] auto render_task(const style_t& style, const plan_t& plan, const task_t& task, const tasks_view& view)
    -> std::string;

/// The dashboard's line: open tasks, those waiting for approval, hours and tokens left.
[[nodiscard]] auto tasks_summary(const style_t& style, const plan_t& plan) -> std::string;

} // namespace heimdall
