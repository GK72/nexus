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

#include "heimdall/tasks.hpp"

#include "heimdall/process.hpp"

#include <fmt/format.h>

#include <algorithm>
#include <array>
#include <exception>
#include <functional>
#include <iterator>
#include <set>
#include <system_error>
#include <utility>

namespace heimdall {

namespace {

constexpr std::size_t KindWidth = 7;
constexpr std::size_t HoursWidth = 5;
constexpr std::size_t StateWidth = 7;
constexpr std::size_t MaxNameWidth = 44;
constexpr std::size_t MinTitleWidth = 12;

[[nodiscard]] auto hours(double value) -> std::string {
    return fmt::format("{:g}h", value);
}

[[nodiscard]] auto kind_color(std::string_view kind) -> std::string_view {
    if (kind == "feature") {
        return "blue";
    }
    if (kind == "debt") {
        return "mauve";
    }
    if (kind == "bug") {
        return "red";
    }
    return "dim";
}

[[nodiscard]] auto state_color(std::string_view state) -> std::string_view {
    if (state == "active") {
        return "accent";
    }
    if (state == "ready") {
        return "green";
    }
    if (state == "blocked") {
        return "orange";
    }
    return state == "done" ? "dim" : "faint";
}

/// Active first, then ready, then blocked; closed ones last; then by id.
[[nodiscard]] auto before(const task_t* lhs, const task_t* rhs) -> bool {
    static constexpr auto Order = std::to_array<std::string_view>({ "active", "ready", "blocked", "done" });
    const auto rank = [](const task_t* task) { return std::ranges::find(Order, task->state) - Order.begin(); };
    return std::pair(rank(lhs), std::string_view(lhs->id)) < std::pair(rank(rhs), std::string_view(rhs->id));
}

/// A line of a tree: the branches before it, the task, and whether it was shown above.
struct entry_t {
    std::string branches;
    const task_t* task = nullptr;
    bool repeat = false;
};

using children_fn = std::function<std::vector<const task_t*>(const task_t&)>;

/// Adds `task` and, unless it is there already, its children below it. Norn refuses cycles.
// NOLINTNEXTLINE(misc-no-recursion): as deep as a dependency chain
void grow(std::vector<entry_t>& out, const task_t* task, const std::string& glyph, const std::string& lead,
          const children_fn& children, std::set<std::string_view>& seen)
{
    const bool repeat = not seen.insert(task->id).second;
    out.push_back({ .branches = glyph, .task = task, .repeat = repeat });
    if (repeat) {
        return;
    }
    auto kids = children(*task);
    std::ranges::sort(kids, before);
    for (std::size_t index = 0; index < kids.size(); ++index) {
        const bool last = index + 1 == kids.size();
        grow(out, kids.at(index), lead + (last ? "└─ " : "├─ "), lead + (last ? "   " : "│  "), children, seen);
    }
}

[[nodiscard]] auto tree(const std::vector<const task_t*>& roots, const children_fn& children) -> std::vector<entry_t> {
    auto sorted = roots;
    std::ranges::sort(sorted, before);
    std::vector<entry_t> out;
    std::set<std::string_view> seen;
    for (const auto* root : sorted) {
        grow(out, root, "", "", children, seen);
    }
    return out;
}

/// The id as shown under project `home`: without the project when it is that one.
[[nodiscard]] auto shown_id(const task_t& task, std::string_view home) -> std::string {
    return task.project == home ? std::string(task.name()) : task.id;
}

/// Columns the names of `entries` take, at most `MaxNameWidth`.
[[nodiscard]] auto name_width(const std::vector<entry_t>& entries, std::string_view home) -> std::size_t {
    std::size_t width = 0;
    for (const auto& entry : entries) {
        width = std::max(width, visible_width(entry.branches + shown_id(*entry.task, home)));
    }
    return std::min(width, MaxNameWidth);
}

/// The lines of a tree: name, kind, hours left, state and title, in columns.
[[nodiscard]] auto tree_lines(const style_t& style, const std::vector<entry_t>& entries, std::string_view home,
                              std::size_t width, std::size_t name_width,
                              const std::function<std::string(const task_t&)>& notes = {})
    -> std::vector<std::string>
{

    std::vector<std::string> lines;
    for (const auto& entry : entries) {
        const auto& task = *entry.task;
        const auto branches = style(entry.branches, "faint");
        const auto room = name_width > visible_width(entry.branches) ? name_width - visible_width(entry.branches) : 1;
        const auto name = ellipsize(shown_id(task, home), room);
        if (entry.repeat) {
            lines.push_back(branches + style(name, "dim") + style("  ↑ above", "faint"));
            continue;
        }
        auto line = pad(branches + style(name, task.open() ? "text" : "dim", task.state == "active"), name_width)
                  + "  " + pad(style(task.kind, kind_color(task.kind)), KindWidth)
                  + " " + style(fmt::format("{:>{}}", task.open() ? hours(task.left()) : "", HoursWidth), "text")
                  + "  " + pad(style(task.state, state_color(task.state)), StateWidth);
        const auto used = visible_width(line) + 2;
        if (width >= used + MinTitleWidth) {
            const auto note = notes ? notes(task) : std::string();
            const auto room_left = width - used - (note.empty() ? 0 : visible_width(note) + 1);
            const auto title = room_left >= MinTitleWidth ? style(ellipsize(task.title, room_left), "dim") : "";
            line += "  " + join(std::vector<std::string>{ note, title }, " ");
        }
        lines.push_back(std::move(line));
    }
    return lines;
}

[[nodiscard]] auto matches(const task_t& task, std::string_view prefix) -> bool {
    return prefix.empty() or task.id == prefix
        or (task.id.starts_with(prefix) and task.id.size() > prefix.size() and task.id.at(prefix.size()) == '/');
}

[[nodiscard]] auto repo_info(const style_t& style, const tasks_view& view, const std::string& project) -> std::string {
    const auto repo = view.repos.find(project);
    return repo != view.repos.end() ? style(home_path(repo->second), "dim") : style("no repository", "orange");
}

/// The open tasks that `task` waits on, the ones they wait on, and so on, each once.
[[nodiscard]] auto open_chain(const plan_t& plan, const task_t& task) -> std::vector<const task_t*> {
    std::vector<const task_t*> chain;
    std::set<std::string_view> seen;
    std::vector<const task_t*> todo { &task };
    while (not todo.empty()) {
        const auto* current = todo.back();
        todo.pop_back();
        for (const auto* need : plan.needs(current->id)) {
            if (need->open() and seen.insert(need->id).second) {
                chain.push_back(need);
                todo.push_back(need);
            }
        }
    }
    return chain;
}

} // namespace

auto task_t::name() const -> std::string_view {
    const std::string_view text = id;
    return text.starts_with(project) and text.size() > project.size() and text.at(project.size()) == '/'
        ? text.substr(project.size() + 1)
        : text;
}

auto plan_t::find(std::string_view id) const -> const task_t* {
    const auto task = std::ranges::find(tasks, id, &task_t::id);
    return task != tasks.end() ? &*task : nullptr;
}

auto plan_t::needs(std::string_view id) const -> std::vector<const task_t*> {
    std::vector<const task_t*> ret;
    for (const auto& dep : deps) {
        if (dep.task == id) {
            if (const auto* need = find(dep.needs)) {
                ret.push_back(need);
            }
        }
    }
    std::ranges::sort(ret, {}, &task_t::id);
    return ret;
}

auto plan_t::needed_by(std::string_view id) const -> std::vector<const task_t*> {
    std::vector<const task_t*> ret;
    for (const auto& dep : deps) {
        if (dep.needs == id) {
            if (const auto* task = find(dep.task)) {
                ret.push_back(task);
            }
        }
    }
    std::ranges::sort(ret, {}, &task_t::id);
    return ret;
}

auto parse_plan(const nova::dyn_value& doc) -> nova::expected<plan_t, nova::error> {
    plan_t plan;
    try {
        for (const auto& item : doc.at("tasks").as_seq()) {
            plan.tasks.push_back({
                .id = item.lookup<std::string>("id"),
                .project = item.lookup<std::string>("project"),
                .kind = item.lookup<std::string>("kind", ""),
                .title = item.lookup<std::string>("title", ""),
                .refs = item.lookup<std::string>("refs", ""),
                .status = item.lookup<std::string>("status"),
                .state = item.lookup<std::string>("state"),
                .added = item.lookup<std::string>("added", ""),
                .closed = item.find<std::string>("closed"),
                .note = item.lookup<std::string>("note", ""),
                .original = item.lookup<double>("original", 0.0),
                .current = item.lookup<double>("current", 0.0),
                .spent = item.lookup<double>("spent", 0.0),
                .moves = item.lookup<int>("moves", 0),
            });
        }
        for (const auto& item : doc.at("deps").as_seq()) {
            plan.deps.push_back({ .task = item.lookup<std::string>("task"), .needs = item.lookup<std::string>("needs") });
        }
    } catch (const std::exception& ex) {
        return nova::wrap_error(std::errc::invalid_argument, ex.what());
    }
    std::ranges::sort(plan.tasks, {}, &task_t::id);
    return plan;
}

auto load_plan(const std::vector<std::string>& command) -> nova::expected<plan_t, nova::error> {
    auto args = command;
    args.insert(args.end(), { "graph", "--format", "json" });
    const auto result = run_process(args, { .timeout = std::chrono::seconds(15) });
    if (not result.success()) {
        auto reason = result.err;
        if (result.timed_out) {
            reason = "timed out";
        } else if (result.status == 127) {
            reason = fmt::format("{} could not start", command.front());
        }
        while (not reason.empty() and (reason.back() == '\n' or reason.back() == ' ')) {
            reason.pop_back();
        }
        if (const auto last = reason.rfind('\n'); last != std::string::npos) {
            reason = reason.substr(last + 1);
        }
        return nova::wrap_error(std::errc::io_error,
                                reason.empty() ? fmt::format("exit status {}", result.status) : reason);
    }
    auto doc = nova::dyn_value::parse(result.out, nova::dyn_value::format_e::json);
    if (not doc) {
        return nova::unexpected<nova::error>{ doc.error() };
    }
    return parse_plan(*doc);
}

auto project_repos(const plan_t& plan, const std::vector<std::filesystem::path>& repos,
                   const std::map<std::string, std::string>& projects) -> std::map<std::string, std::string>
{
    std::map<std::string, std::string> by_name;
    for (const auto& repo : repos) {
        by_name.emplace(repo.filename().string(), repo.string());
    }
    std::map<std::string, std::string> ret;
    for (const auto& task : plan.tasks) {
        if (ret.contains(task.project)) {
            continue;
        }
        const auto named = projects.find(task.project);
        const auto name = named != projects.end() ? named->second : task.project;
        if (name.contains('/') or name.starts_with('~')) {
            ret.emplace(task.project, expand_home(name));
        } else if (const auto repo = by_name.find(name); repo != by_name.end()) {
            ret.emplace(task.project, repo->second);
        }
    }
    return ret;
}

auto render_tasks(const style_t& style, const plan_t& plan, std::string_view prefix, const tasks_view& view)
    -> std::string
{
    std::map<std::string, std::vector<const task_t*>> by_project;
    for (const auto& task : plan.tasks) {
        if (task.open() and matches(task, prefix)) {
            by_project[task.project].push_back(&task);
        }
    }
    const auto separator = style(" · ", "faint");

    double total = 0.0;
    std::size_t count = 0;
    for (const auto& [project, tasks] : by_project) {
        count += tasks.size();
        for (const auto* task : tasks) {
            total += task->left();
        }
    }
    const auto totals = style(fmt::format("{} open", count), "text") + separator
                      + style(fmt::format("{} project{}", by_project.size(), by_project.size() == 1 ? "" : "s"), "dim")
                      + separator + style(hours(total) + " left", "dim");

    std::vector<std::string> out { "", spread(style("Tasks", "title", true), totals, view.width) };
    if (by_project.empty()) {
        out.emplace_back();
        out.push_back(style(prefix.empty() ? "no open tasks" : fmt::format("no open task under {}", prefix), "dim"));
    }

    struct section_t {
        std::string project;
        std::string info;
        std::vector<entry_t> entries;
        std::function<std::string(const task_t&)> notes;
    };
    std::vector<section_t> sections;
    std::size_t names = 0;
    for (const auto& [project, tasks] : by_project) {
        std::set<std::string_view> chosen;
        for (const auto* task : tasks) {
            chosen.insert(task->id);
        }
        const auto inside = [&](const task_t* task) {
            return task->open() and task->project == project and chosen.contains(task->id);
        };
        std::vector<const task_t*> roots;
        double left = 0.0;
        std::size_t ready = 0;
        std::size_t blocked = 0;
        for (const auto* task : tasks) {
            left += task->left();
            ready += task->state == "ready" or task->state == "active" ? 1 : 0;
            blocked += task->state == "blocked" ? 1 : 0;
            if (std::ranges::none_of(plan.needs(task->id), inside)) {
                roots.push_back(task);
            }
        }
        const auto children = [&](const task_t& task) {
            std::vector<const task_t*> kids;
            std::ranges::copy_if(plan.needed_by(task.id), std::back_inserter(kids), inside);
            return kids;
        };
        const auto elsewhere = [&plan, &style, project, chosen](const task_t& task) {
            const auto inside = [&](const task_t* need) {
                return need->open() and need->project == project and chosen.contains(need->id);
            };
            std::vector<std::string> ids;
            for (const auto* need : plan.needs(task.id)) {
                if (need->open() and not inside(need)) {
                    ids.push_back(shown_id(*need, project));
                }
            }
            return ids.empty() ? std::string() : style("needs " + join(ids, ", "), "orange");
        };

        std::vector<std::string> info { repo_info(style, view, project),
                                        style(fmt::format("{} open", tasks.size()), "text") };
        if (ready > 0) {
            info.push_back(style(fmt::format("{} ready", ready), "green"));
        }
        if (blocked > 0) {
            info.push_back(style(fmt::format("{} blocked", blocked), "orange"));
        }
        info.push_back(style(hours(left) + " left", "dim"));

        sections.push_back({ .project = project, .info = join(info, separator), .entries = tree(roots, children),
                             .notes = elsewhere });
        names = std::max(names, name_width(sections.back().entries, project));
    }
    for (const auto& section : sections) {
        out.emplace_back();
        out.push_back(rule(style, section.project, view.width, section.info));
        std::ranges::move(tree_lines(style, section.entries, section.project, view.width, names, section.notes),
                          std::back_inserter(out));
    }

    std::string text;
    for (const auto& line : out) {
        text += (line.empty() ? "" : "  " + line) + "\n";
    }
    return text;
}

auto render_task(const style_t& style, const plan_t& plan, const task_t& task, const tasks_view& view) -> std::string {
    const auto separator = style(" · ", "faint");
    const auto fact = [&](std::string_view label, const std::string& value) {
        return style(label, "dim") + " " + style(value, "text");
    };

    std::vector<std::string> out { "", spread(style(task.id, "title", true),
                                              style(task.kind, kind_color(task.kind)) + separator
                                                  + style(task.state, state_color(task.state), true),
                                              view.width) };
    for (const auto& line : wrap(task.title, view.width)) {
        out.push_back(style(line, "text"));
    }
    out.emplace_back();

    std::vector<std::string> plan_facts { fact("estimate", hours(task.current)) };
    if (task.original != task.current) {
        plan_facts.push_back(fact("originally", hours(task.original)));
    }
    plan_facts.push_back(fact("spent", hours(task.spent)));
    if (task.open()) {
        plan_facts.push_back(fact("left", hours(task.left())));
    }
    if (task.moves > 0) {
        plan_facts.push_back(fact("moved", fmt::format("{}×", task.moves)));
    }
    out.push_back(join(plan_facts, separator));
    std::vector<std::string> dates { fact("added", task.added) };
    if (task.closed) {
        dates.push_back(fact(task.status, *task.closed));
    }
    out.push_back(join(dates, separator));
    out.push_back(style("repository ", "dim") + repo_info(style, view, task.project));

    const auto open_only = [](std::vector<const task_t*> tasks) {
        std::erase_if(tasks, [](const task_t* t) { return not t->open(); });
        return tasks;
    };
    const auto needs = plan.needs(task.id);
    if (not needs.empty()) {
        const auto chain = open_chain(plan, task);
        double left = 0.0;
        for (const auto* need : chain) {
            left += need->left();
        }
        const auto info = chain.empty()
            ? style("all done", "green")
            : style(fmt::format("{} open", chain.size()), "orange") + separator
                  + style(fmt::format("{} before it can start", hours(left)), "dim");
        out.emplace_back();
        out.push_back(rule(style, "waits on", view.width, info));
        const children_fn up = [&](const task_t& t) { return t.open() ? plan.needs(t.id) : std::vector<const task_t*>{}; };
        const auto entries = tree(needs, up);
        std::ranges::move(tree_lines(style, entries, task.project, view.width, name_width(entries, task.project)),
                          std::back_inserter(out));
    }

    const auto dependents = plan.needed_by(task.id);
    if (not dependents.empty()) {
        out.emplace_back();
        out.push_back(rule(style, "unblocks", view.width,
                           style(fmt::format("{} open", open_only(dependents).size()), "dim")));
        const children_fn down = [&](const task_t& t) { return open_only(plan.needed_by(t.id)); };
        const auto entries = tree(dependents, down);
        std::ranges::move(tree_lines(style, entries, task.project, view.width, name_width(entries, task.project)),
                          std::back_inserter(out));
    }

    for (const auto& [title, text] : { std::pair<std::string_view, const std::string&>{ "refs", task.refs },
                                       std::pair<std::string_view, const std::string&>{ "note", task.note } })
    {
        if (text.empty()) {
            continue;
        }
        out.emplace_back();
        out.push_back(rule(style, title, view.width));
        for (const auto& line : wrap(text, view.width)) {
            out.push_back(style(line, "text"));
        }
    }

    std::string text;
    for (const auto& line : out) {
        text += (line.empty() ? "" : "  " + line) + "\n";
    }
    return text;
}

} // namespace heimdall
