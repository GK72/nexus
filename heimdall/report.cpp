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

#include "heimdall/report.hpp"

#include <fmt/format.h>

#include <algorithm>
#include <array>
#include <map>
#include <ranges>
#include <string_view>
#include <tuple>

namespace heimdall {

namespace {

constexpr std::size_t MaxNameWidth = 36;

/// A row of a repository's block: glyph, name, marks, note.
using row_t = std::array<std::string, 4>;

[[nodiscard]] auto plural(int count, std::string_view word) -> std::string {
    return fmt::format("{} {}{}", count, word, count == 1 ? "" : "s");
}

[[nodiscard]] auto summary(const style_t& style, const std::vector<repo_t>& repos, bool fetched, std::int64_t now)
    -> std::string
{
    const auto count = [&](auto pred) { return static_cast<int>(std::ranges::count_if(repos, pred)); };
    const auto sum = [&](auto value) {
        int total = 0;
        for (const auto& repo : repos) {
            total += value(repo);
        }
        return total;
    };

    std::vector<std::string> parts { style(plural(static_cast<int>(repos.size()), "repo"), "text") };
    const int attention = count(&repo_t::attention);
    parts.push_back(attention > 0 ? style(fmt::format("{} need attention", attention), "yellow")
                                  : style("all clear", "green"));
    const auto add = [&](int value, std::string_view word, std::string_view color) {
        if (value > 0) {
            parts.push_back(style(fmt::format("{} {}", value, word), color));
        }
    };
    add(count(&repo_t::dirty), "dirty", "yellow");
    add(sum([](const repo_t& repo) { return repo.unpushed; }), "unpushed", "orange");
    add(sum([](const repo_t& repo) { return static_cast<int>(repo.stashes.size()); }), "stashed", "mauve");
    add(sum([](const repo_t& repo) { return repo.gone(); }), "upstream gone", "dim");
    add(count([](const repo_t& repo) { return repo.error or repo.fetch_error; }), "failed", "red");
    parts.push_back(style(fetch_info(repos, fetched, now), "dim"));
    return join(parts, style(" · ", "faint"));
}

[[nodiscard]] auto legend(const style_t& style) -> std::string {
    static constexpr auto Items = std::to_array<std::tuple<std::string_view, std::string_view, std::string_view>>({
        { "!", "red", "conflict" }, { "+", "green", "staged" }, { "~", "yellow", "modified" },
        { "?", "dim", "untracked" }, { "↑", "orange", "unpushed" }, { "↓", "blue", "behind" },
        { "≡", "mauve", "stash" }, { "°", "faint", "no remote" },
    });
    std::vector<std::string> parts;
    for (const auto& [symbol, color, word] : Items) {
        parts.push_back(style(symbol, color) + style(fmt::format(" {}", word), "faint"));
    }
    return join(parts, "  ");
}

[[nodiscard]] auto repo_rows(const style_t& style, const repo_t& repo, std::int64_t now) -> std::vector<row_t> {
    std::vector<row_t> rows;
    if (repo.error) {
        rows.push_back({ style("✗", "red"), style("error", "red"), "", style(*repo.error, "red") });
    }
    if (repo.fetch_error) {
        rows.push_back({ style("✗", "red"), style("fetch failed", "red"), "", style(*repo.fetch_error, "dim") });
    }

    std::map<std::string_view, const branch_t*> by_name;
    for (const auto& branch : repo.branches) {
        by_name[branch.name] = &branch;
    }
    const auto repo_prefix = home_path(repo.path) + "/";

    for (const auto& tree : repo.worktrees) {
        const auto* branch = tree.branch ? by_name[*tree.branch] : nullptr;
        const bool notable = tree.status.dirty() or (branch != nullptr and branch->notable());
        if (tree.main and tree.branch and not notable) {
            continue;
        }
        auto where = tree.main ? std::string() : home_path(tree.path);
        if (where.starts_with(repo_prefix)) {
            where = where.substr(repo_prefix.size());
        }
        std::vector<std::string> notes { where };
        if (tree.branch and not tree.status.upstream and not repo.local_only()) {
            notes.emplace_back("no upstream");
        }
        if (branch != nullptr and branch->gone) {
            notes.emplace_back("upstream gone");
        }
        if (tree.locked) {
            notes.emplace_back("locked");
        }
        const auto name = tree.branch ? style(*tree.branch, "text", tree.main)
                                      : style(fmt::format("detached @{}", tree.head), "pink");
        const auto marks = badges(style, {
            .conflicts = tree.status.conflicts, .staged = tree.status.staged, .modified = tree.status.modified,
            .untracked = tree.status.untracked, .unpushed = branch != nullptr ? branch->unpushed : 0,
            .behind = tree.status.behind,
        });
        rows.push_back({ style(tree.main ? "⌂" : "⎇", tree.main ? "accent" : "title"), name, marks,
                         style(join(notes, "  "), "dim") });
    }

    std::vector<const branch_t*> others;
    for (const auto& branch : repo.branches) {
        const bool checked_out = std::ranges::any_of(repo.worktrees, [&](const worktree_t& tree) {
            return tree.branch == branch.name;
        });
        if (not checked_out and branch.notable()) {
            others.push_back(&branch);
        }
    }
    std::ranges::sort(others, std::greater{}, &branch_t::committed);
    for (const auto* branch : others) {
        std::vector<std::string> notes;
        if (branch->gone) {
            notes.push_back(style("upstream gone", "dim"));
        } else if (not branch->upstream and not repo.local_only()) {
            notes.push_back(style("no upstream", "dim"));
        }
        notes.push_back(style(ago(now - branch->committed), "faint"));
        rows.push_back({ style("•", "dim"), style(branch->name, "text"),
                         badges(style, { .unpushed = branch->unpushed, .behind = branch->behind }), join(notes, "  ") });
    }

    for (const auto& stash : repo.stashes) {
        rows.push_back({ style("≡", "mauve"), style(stash.ref, "mauve"), style(ago(now - stash.created), "faint"),
                         style(ellipsize(stash.message, 60), "dim") });
    }
    return rows;
}

[[nodiscard]] auto render_repo(const style_t& style, const repo_t& repo, std::int64_t now) -> std::vector<std::string> {
    std::vector<std::string> meta { home_path(repo.path) };
    if (repo.local_only()) {
        meta.emplace_back("no remote");
    } else if (not repo.fetch_error) {
        meta.push_back(repo.last_fetch ? fmt::format("fetched {} ago", ago(now - *repo.last_fetch)) : "never fetched");
    }
    std::vector<std::string> out {
        fmt::format(" {}{}  {}", style("▌", repo.attention() ? "yellow" : "green"), style(repo.name, "accent", true),
                    style(join(meta, "  "), "dim")),
    };

    const auto rows = repo_rows(style, repo, now);
    std::size_t name_width = 0;
    std::size_t marks_width = 0;
    for (const auto& row : rows) {
        name_width = std::max(name_width, visible_width(row[1]));
        marks_width = std::max(marks_width, visible_width(row[2]));
    }
    name_width = std::min(name_width, MaxNameWidth);
    for (std::size_t index = 0; index < rows.size(); ++index) {
        const auto& [glyph, name, marks, note] = rows[index];
        auto line = fmt::format("  {} {} {}  {}  {}", style(index + 1 == rows.size() ? "└" : "├", "faint"), glyph,
                                pad(name, name_width), pad(marks, marks_width), note);
        while (line.ends_with(' ')) {
            line.pop_back();
        }
        out.push_back(std::move(line));
    }
    return out;
}

} // namespace

auto badges(const style_t& style, const marks_t& marks) -> std::string {
    const auto items = std::to_array<std::tuple<int, std::string_view, std::string_view>>({
        { marks.conflicts, "!", "red" }, { marks.staged, "+", "green" }, { marks.modified, "~", "yellow" },
        { marks.untracked, "?", "dim" }, { marks.unpushed, "↑", "orange" }, { marks.behind, "↓", "blue" },
        { marks.stashes, "≡", "mauve" },
    });
    std::vector<std::string> parts;
    for (const auto& [count, symbol, color] : items) {
        if (count > 0) {
            parts.push_back(style(fmt::format("{}{}", symbol, count), color, color == "red"));
        }
    }
    return join(parts, " ");
}

auto fetch_info(const std::vector<repo_t>& repos, bool fetched, std::int64_t now) -> std::string {
    if (fetched) {
        return "fetched just now";
    }
    std::optional<std::int64_t> oldest;
    for (const auto& repo : repos) {
        if (not repo.local_only() and repo.last_fetch) {
            oldest = std::min(oldest.value_or(*repo.last_fetch), *repo.last_fetch);
        }
    }
    return oldest ? fmt::format("oldest fetch {} ago", ago(now - *oldest)) : "never fetched";
}

auto render_report(const style_t& style, const std::vector<repo_t>& repos, bool fetched, std::int64_t now)
    -> std::string
{
    std::vector<std::string> out {
        "",
        fmt::format("{} {}", style("Heimdall git report:", "title", true), summary(style, repos, fetched, now)),
        "",
    };
    for (const auto& repo : repos) {
        if (repo.attention()) {
            std::ranges::move(render_repo(style, repo, now), std::back_inserter(out));
            out.emplace_back();
        }
    }
    out.push_back(" " + legend(style));
    std::string text;
    for (const auto& line : out) {
        text += line + "\n";
    }
    return text;
}

auto brief_rows(const style_t& style, const std::vector<repo_t>& repos) -> std::vector<std::string> {
    std::vector<row_t> rows;
    for (const auto& repo : repos) {
        if (not repo.attention()) {
            continue;
        }
        marks_t marks { .unpushed = repo.unpushed, .behind = repo.behind(), .stashes = static_cast<int>(repo.stashes.size()) };
        for (const auto& tree : repo.worktrees) {
            marks.conflicts += tree.status.conflicts;
            marks.staged += tree.status.staged;
            marks.modified += tree.status.modified;
            marks.untracked += tree.status.untracked;
        }
        std::vector<std::string> notes;
        if (const auto linked = static_cast<int>(repo.worktrees.size()) - 1; linked > 0) {
            notes.push_back(style(fmt::format("+{} worktree{}", linked, linked == 1 ? "" : "s"), "dim"));
        }
        if (const int gone = repo.gone(); gone > 0) {
            notes.push_back(style(fmt::format("{} upstream gone", gone), "dim"));
        }
        if (repo.fetch_error or repo.error) {
            notes.push_back(style(repo.fetch_error ? "fetch failed" : "error", "red"));
        }
        std::string branch;
        if (not repo.worktrees.empty()) {
            const auto& main = repo.worktrees.front();
            branch = main.branch ? *main.branch : "@" + main.head;
        }
        rows.push_back({ style(repo.name, "accent"), style(ellipsize(branch, 24), "dim"), badges(style, marks),
                         join(notes, style(" · ", "faint")) });
    }

    std::array<std::size_t, 3> widths {};
    for (const auto& row : rows) {
        for (std::size_t column = 0; column < widths.size(); ++column) {
            widths.at(column) = std::max(widths.at(column), visible_width(row.at(column)));
        }
    }
    std::vector<std::string> lines;
    for (const auto& [name, branch, marks, notes] : rows) {
        auto line = fmt::format("{}  {}  {}  {}", pad(name, widths[0]), pad(branch, widths[1]), pad(marks, widths[2]),
                                notes);
        while (line.ends_with(' ')) {
            line.pop_back();
        }
        lines.push_back(std::move(line));
    }
    return lines;
}

auto render_brief(const style_t& style, const std::vector<repo_t>& repos, bool fetched, std::int64_t now)
    -> std::string
{
    std::string text = fmt::format("{} {}\n", style("Heimdall git brief:", "title", true),
                                   summary(style, repos, fetched, now));
    for (const auto& row : brief_rows(style, repos)) {
        text += "  " + row + "\n";
    }
    return text;
}

} // namespace heimdall
