/**
 * Part of Nexus Library
 *
 * @brief   The state of the tracked Git repositories: what needs attention.
 *
 * @author      Gábor Krisztián Girhiny
 * @created     2026-11-10
 * @updated     2026-11-10
 * @status      experimental
 * @reviewed    none
 * SPDX-License-Identifier: Unlicense
 */

#include "heimdall/git.hpp"

#include "heimdall/process.hpp"
#include "heimdall/style.hpp"

#include <fmt/format.h>

#include <algorithm>
#include <atomic>
#include <cctype>
#include <charconv>
#include <chrono>
#include <iostream>
#include <map>
#include <ranges>
#include <set>
#include <stdexcept>
#include <thread>

#include <fnmatch.h>

namespace heimdall {

namespace {

constexpr auto FetchTimeout = std::chrono::seconds(60);

/// A git command that failed: its complaint.
class git_error : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

/// The output of `git -C repo args...`; throws `git_error` if it fails.
auto git(const std::string& repo, std::vector<std::string> args, bool check = true) -> std::string {
    args.insert(args.begin(), { "git", "-C", repo });
    auto result = run_process(args, {
        .directory = {},
        .env = { { "GIT_OPTIONAL_LOCKS", "0" }, { "LC_ALL", "C" } },
    });
    if (check and not result.success()) {
        const auto line = result.err.substr(0, result.err.find('\n'));
        throw git_error(line.empty() ? fmt::format("git {} failed", args[3]) : line);
    }
    return std::move(result.out);
}

[[nodiscard]] auto lines(std::string_view text) -> std::vector<std::string_view> {
    std::vector<std::string_view> ret;
    for (auto&& line : std::views::split(text, '\n')) {
        ret.emplace_back(line.begin(), line.end());
    }
    if (not ret.empty() and ret.back().empty()) {
        ret.pop_back();
    }
    return ret;
}

[[nodiscard]] auto fields(std::string_view line, char separator) -> std::vector<std::string_view> {
    std::vector<std::string_view> ret;
    for (auto&& field : std::views::split(line, separator)) {
        ret.emplace_back(field.begin(), field.end());
    }
    return ret;
}

template <typename T>
[[nodiscard]] auto to_number(std::string_view text) -> T {
    T value {};
    std::from_chars(text.data(), text.data() + text.size(), value);
    return value;
}

[[nodiscard]] auto trim(std::string_view text) -> std::string_view {
    while (not text.empty() and std::isspace(static_cast<unsigned char>(text.back())) != 0) {
        text.remove_suffix(1);
    }
    while (not text.empty() and std::isspace(static_cast<unsigned char>(text.front())) != 0) {
        text.remove_prefix(1);
    }
    return text;
}

[[nodiscard]] auto optional_text(std::string_view text) -> std::optional<std::string> {
    return text.empty() ? std::nullopt : std::optional<std::string>(text);
}

void fetch(repo_t& repo) {
    auto result = run_process({ "git", "-C", repo.path, "fetch", "--all", "--prune", "--quiet" }, {
        .directory = {},
        .env = { { "GIT_TERMINAL_PROMPT", "0" }, { "LC_ALL", "C" } },
        .timeout = FetchTimeout,
        .new_session = true,
    });
    if (result.timed_out) {
        repo.fetch_error = fmt::format("timed out after {}s", FetchTimeout.count());
    } else if (not result.success()) {
        const auto reason = fetch_reason(result.err);
        repo.fetch_error = reason.empty() ? fmt::format("exit status {}", result.status) : reason;
    }
}

void read_last_fetch(repo_t& repo) {
    const auto common = std::string(trim(git(repo.path, { "rev-parse", "--path-format=absolute", "--git-common-dir" })));
    std::error_code ec;
    const auto written = std::filesystem::last_write_time(std::filesystem::path(common) / "FETCH_HEAD", ec);
    if (not ec) {
        const auto system = std::chrono::clock_cast<std::chrono::system_clock>(written);
        repo.last_fetch = std::chrono::duration_cast<std::chrono::seconds>(system.time_since_epoch()).count();
    }
}

void read_branches(repo_t& repo) {
    repo.branches = parse_branches(git(repo.path, { "for-each-ref", fmt::format("--format={}", BranchFormat), "refs/heads" }));
    if (repo.remotes.empty() or repo.branches.empty()) {
        return;
    }
    const auto count = [&](std::vector<std::string> args) {
        return to_number<int>(trim(git(repo.path, std::move(args))));
    };
    repo.unpushed = count({ "rev-list", "--count", "--branches", "--not", "--remotes" });
    if (repo.unpushed == 0) {
        return;
    }
    for (auto& branch : repo.branches) {
        branch.unpushed = count({ "rev-list", "--count", "refs/heads/" + branch.name, "--not", "--remotes" });
    }
}

[[nodiscard]] auto inspect(const std::filesystem::path& path, bool do_fetch) -> repo_t {
    repo_t repo { .name = path.filename().string(), .path = path.string() };
    try {
        for (const auto line : lines(git(repo.path, { "remote" }))) {
            repo.remotes.emplace_back(line);
        }
        if (do_fetch and not repo.local_only()) {
            fetch(repo);
        }
        read_last_fetch(repo);
        repo.worktrees = parse_worktrees(git(repo.path, { "worktree", "list", "--porcelain" }));
        for (auto& tree : repo.worktrees) {
            tree.status = parse_status(git(tree.path, { "status", "--porcelain=v2", "--branch", "--untracked-files=normal" }));
        }
        read_branches(repo);
        repo.stashes = parse_stashes(git(repo.path, { "stash", "list", fmt::format("--format={}", StashFormat) }, false));
    } catch (const std::exception& ex) {
        repo.error = ex.what();
    }
    return repo;
}

} // namespace

auto repo_t::dirty() const -> bool {
    return std::ranges::any_of(worktrees, [](const worktree_t& tree) { return tree.status.dirty(); });
}

auto repo_t::behind() const -> int {
    int sum = 0;
    for (const auto& branch : branches) {
        sum += branch.behind;
    }
    return sum;
}

auto repo_t::gone() const -> int {
    return static_cast<int>(std::ranges::count_if(branches, &branch_t::gone));
}

auto repo_t::attention() const -> bool {
    return error or fetch_error or dirty() or not stashes.empty()
        or std::ranges::any_of(branches, &branch_t::notable);
}

auto discover(const std::vector<std::string>& roots, const std::vector<std::string>& exclude,
              const std::vector<std::string>& repositories) -> std::vector<std::filesystem::path>
{
    namespace fs = std::filesystem;
    std::error_code ec;
    std::vector<fs::path> found;
    const auto add = [&](const fs::path& path) {
        if (std::ranges::find(found, path) == found.end()) {
            found.push_back(path);
        }
    };
    const auto is_repo = [&](const fs::path& path) { return fs::is_directory(path / ".git", ec); };
    const auto excluded = [&](const fs::path& path) {
        return std::ranges::any_of(exclude, [&](const std::string& glob) {
            return ::fnmatch(glob.c_str(), path.filename().c_str(), 0) == 0;
        });
    };
    const auto resolve = [&](const std::string& raw) -> std::optional<fs::path> {
        auto path = fs::weakly_canonical(expand_home(raw), ec);
        if (ec or not fs::is_directory(path, ec)) {
            std::cerr << fmt::format("heimdall: not a directory: {}\n", raw);
            return std::nullopt;
        }
        return path;
    };

    for (const auto& raw : roots) {
        const auto root = resolve(raw);
        if (not root) {
            continue;
        }
        if (is_repo(*root)) {
            add(*root);
            continue;
        }
        std::vector<fs::path> children;
        for (const auto& entry : fs::directory_iterator(*root, ec)) {
            if (entry.is_directory(ec) and is_repo(entry.path()) and not excluded(entry.path())) {
                children.push_back(entry.path());
            }
        }
        std::ranges::sort(children);
        std::ranges::for_each(children, add);
    }
    for (const auto& raw : repositories) {
        if (const auto path = resolve(raw); path and is_repo(*path)) {
            add(*path);
        } else if (path) {
            std::cerr << fmt::format("heimdall: not a repository: {}\n", raw);
        }
    }
    return found;
}

auto survey(const config_t& config, const survey_options& options) -> std::vector<repo_t> {
    auto exclude = config.exclude_from_roots;
    exclude.insert(exclude.end(), options.exclude_from_roots.begin(), options.exclude_from_roots.end());
    auto repositories = config.repositories;
    repositories.insert(repositories.end(), options.repositories.begin(), options.repositories.end());
    const auto paths = discover(config.roots, exclude, repositories);

    std::vector<repo_t> repos(paths.size());
    std::atomic<std::size_t> next = 0;
    {
        std::vector<std::jthread> workers;
        const auto count = std::clamp<std::size_t>(static_cast<std::size_t>(std::max(options.jobs, 1)), 1, paths.size());
        for (std::size_t worker = 0; worker < count; ++worker) {
            workers.emplace_back([&] {
                for (auto index = next++; index < paths.size(); index = next++) {
                    repos[index] = inspect(paths[index], options.fetch);
                }
            });
        }
    }

    std::ranges::sort(repos, {}, [](const repo_t& repo) {
        auto key = repo.name;
        std::ranges::transform(key, key.begin(), [](unsigned char chr) { return std::tolower(chr); });
        return key;
    });
    return repos;
}

auto parse_worktrees(std::string_view text) -> std::vector<worktree_t> {
    std::vector<worktree_t> trees;
    std::map<std::string, std::string, std::less<>> entry;
    std::size_t index = 0;                  // Git lists the main worktree first
    const auto flush = [&] {
        if (entry.empty()) {
            return;
        }
        if (not entry.contains("bare") and not entry.contains("prunable")) {
            worktree_t tree { .path = entry["worktree"], .head = entry["HEAD"].substr(0, 7) };
            if (const auto& ref = entry["branch"]; not ref.empty()) {
                tree.branch = ref.starts_with("refs/heads/") ? ref.substr(11) : ref;
            }
            tree.main = index == 0;
            tree.locked = entry.contains("locked");
            trees.push_back(std::move(tree));
        }
        ++index;
        entry.clear();
    };
    for (const auto line : lines(text)) {
        if (line.empty()) {
            flush();
            continue;
        }
        const auto space = line.find(' ');
        entry[std::string(line.substr(0, space))] =
            space == std::string_view::npos ? "" : std::string(line.substr(space + 1));
    }
    flush();
    return trees;
}

auto parse_status(std::string_view text) -> status_t {
    status_t status;
    for (const auto line : lines(text)) {
        if (line.starts_with("# branch.upstream ")) {
            status.upstream = std::string(line.substr(18));
        } else if (line.starts_with("# branch.ab ")) {
            const auto parts = fields(line, ' ');
            if (parts.size() >= 4) {
                status.ahead = to_number<int>(parts[2].substr(1));
                status.behind = to_number<int>(parts[3].substr(1));
            }
        } else if (line.starts_with("1 ") or line.starts_with("2 ")) {
            status.staged += line.size() > 2 and line[2] != '.' ? 1 : 0;
            status.modified += line.size() > 3 and line[3] != '.' ? 1 : 0;
        } else if (line.starts_with("u ")) {
            ++status.conflicts;
        } else if (line.starts_with("? ")) {
            ++status.untracked;
        }
    }
    return status;
}

auto parse_branches(std::string_view text) -> std::vector<branch_t> {
    std::vector<branch_t> branches;
    for (const auto line : lines(text)) {
        const auto parts = fields(line, '\0');
        if (parts.size() < 7) {
            continue;
        }
        branch_t branch {
            .name = std::string(parts[0]),
            .head = std::string(parts[1]),
            .subject = std::string(parts[6]),
            .committed = to_number<std::int64_t>(parts[2]),
            .upstream = optional_text(parts[3]),
            .worktree = optional_text(parts[5]),
            .gone = parts[4] == "gone",
        };
        for (const auto item : fields(parts[4], ',')) {
            const auto words = fields(trim(item), ' ');
            if (words.size() == 2 and words[0] == "ahead") {
                branch.ahead = to_number<int>(words[1]);
            } else if (words.size() == 2 and words[0] == "behind") {
                branch.behind = to_number<int>(words[1]);
            }
        }
        branches.push_back(std::move(branch));
    }
    return branches;
}

auto parse_stashes(std::string_view text) -> std::vector<stash_t> {
    std::vector<stash_t> stashes;
    for (const auto line : lines(text)) {
        const auto parts = fields(line, '\0');
        if (parts.size() < 3) {
            continue;
        }
        stashes.push_back({
            .ref = std::string(parts[0]),
            .created = to_number<std::int64_t>(parts[1]),
            .message = std::string(parts[2]),
        });
    }
    return stashes;
}

auto fetch_reason(std::string_view stderr_text) -> std::string {
    std::vector<std::string_view> kept;
    for (const auto line : lines(stderr_text)) {
        if (const auto trimmed = trim(line); not trimmed.empty()) {
            kept.push_back(trimmed);
        }
    }
    static constexpr auto Advice = std::to_array<std::string_view>({
        "fatal:", "error:", "Please make sure", "and the repository",
    });
    const auto starts_with_any = [](std::string_view line, auto prefixes) {
        return std::ranges::any_of(prefixes, [&](std::string_view prefix) { return line.starts_with(prefix); });
    };
    for (const auto line : kept) {
        if (not starts_with_any(line, Advice)) {
            return std::string(line);
        }
    }
    for (const auto line : kept) {
        if (starts_with_any(line, std::to_array<std::string_view>({ "fatal:", "error:" }))) {
            return std::string(line);
        }
    }
    return kept.empty() ? std::string() : std::string(kept.front());
}

} // namespace heimdall
