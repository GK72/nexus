/**
 * Part of Nexus Library
 *
 * @brief   The heimdall CLI: the dashboard, and the views of its parts.
 *
 * @author      Gábor Krisztián Girhiny
 * @created     2026-11-10
 * @updated     2026-11-14
 * @status      experimental
 * @reviewed    none
 * SPDX-License-Identifier: Unlicense
 */

#include "heimdall/config.hpp"
#include "heimdall/dashboard.hpp"
#include "heimdall/git.hpp"
#include "heimdall/heimdall.hpp"
#include "heimdall/memory.hpp"
#include "heimdall/report.hpp"
#include "heimdall/tasks.hpp"

#include <libnxs/tty.hpp>

#include <fmt/format.h>

#include <charconv>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace {

constexpr std::string_view Usage = R"(usage: heimdall [-f] [--repos N] [--width N] [--color WHEN]
       heimdall git [-f] [-b] [--check] [--exclude-from-roots GLOB]... [-j N] [--color WHEN] [PATH...]
       heimdall tasks [-a] [--width N] [--color WHEN] [ID | PREFIX]
       heimdall memory [--width N] [--color WHEN]

The dashboard; with `git`, the report of the tracked repositories; with `tasks`,
the open tasks of norn per project, as trees of what unblocks what, or one task
in full; with `memory`, Claude's memory files, their sizes and how often
sessions read them.

  -f, --fetch                  fetch every repository first
  --repos N                    dashboard: repositories listed at most (default 8)
  --width N                    dashboard, tasks, memory: columns (default: the terminal's, at most 100)
  -b, --brief                  git: one line per repository that needs attention
  --check                      git: exit status 1 if anything needs attention
  -a, --all                    tasks: shelved projects too
  --exclude-from-roots GLOB    git: leave out repositories of the roots; repeatable
  -j, --jobs N                 repositories inspected at once (default 8)
  --color WHEN                 auto (default), always or never
  -h, --help                   this usage
  --version                    the version
)";

constexpr int ExitUsage = 2;

struct options_t {
    std::string command;                    ///< Empty: the dashboard
    bool fetch = false;
    bool brief = false;
    bool check = false;
    bool all = false;
    int jobs = 8;
    std::size_t repos = 8;
    std::optional<std::size_t> width;
    heimdall::color_mode color = heimdall::color_mode::automatic;
    std::vector<std::string> paths;         ///< git: repositories; tasks: an id or a prefix
    std::vector<std::string> exclude_from_roots;
};

/// A command-line mistake, reported with the usage.
struct usage_error {
    std::string message;
};

template <typename T>
[[nodiscard]] auto number(std::string_view option, std::string_view text) -> T {
    T value {};
    const auto [end, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (ec != std::errc{} or end != text.data() + text.size() or value <= 0) {
        throw usage_error { fmt::format("{} takes a positive number, not '{}'", option, text) };
    }
    return value;
}

[[nodiscard]] auto parse_args(std::span<const std::string_view> args) -> std::optional<options_t> {
    options_t options;
    std::size_t index = 0;
    if (not args.empty() and not args.front().starts_with('-')) {
        options.command = args.front();
        if (options.command != "git" and options.command != "tasks" and options.command != "memory") {
            throw usage_error { fmt::format("unknown command: {}", options.command) };
        }
        ++index;
    }
    const auto value = [&](std::string_view option) -> std::string_view {
        if (++index >= args.size()) {
            throw usage_error { fmt::format("{} needs a value", option) };
        }
        return args[index];
    };
    const auto git_only = [&](std::string_view option) {
        if (options.command != "git" and options.command != "tasks") {
            throw usage_error { fmt::format("{} is an option of `heimdall git`", option) };
        }
    };
    const auto dashboard_only = [&](std::string_view option) {
        if (not options.command.empty()) {
            throw usage_error { fmt::format("{} is an option of the dashboard", option) };
        }
    };
    const auto not_tasks = [&](std::string_view option) {
        if (options.command == "tasks" or options.command == "memory") {
            throw usage_error { fmt::format("{} is not an option of `heimdall {}`", option, options.command) };
        }
    };

    for (; index < args.size(); ++index) {
        const auto arg = args[index];
        if (arg == "-h" or arg == "--help") {
            fmt::print("{}", Usage);
            return std::nullopt;
        } else if (arg == "--version") {
            fmt::println("heimdall {}", heimdall::version());
            return std::nullopt;
        } else if (arg == "-f" or arg == "--fetch") {
            not_tasks(arg);
            options.fetch = true;
        } else if (arg == "-b" or arg == "--brief") {
            git_only(arg);
            options.brief = true;
        } else if (arg == "--check") {
            git_only(arg);
            options.check = true;
        } else if (arg == "-a" or arg == "--all") {
            if (options.command != "tasks") {
                throw usage_error { fmt::format("{} is an option of `heimdall tasks`", arg) };
            }
            options.all = true;
        } else if (arg == "--exclude-from-roots") {
            git_only(arg);
            options.exclude_from_roots.emplace_back(value(arg));
        } else if (arg == "-j" or arg == "--jobs") {
            not_tasks(arg);
            options.jobs = number<int>(arg, value(arg));
        } else if (arg == "--repos") {
            dashboard_only(arg);
            options.repos = number<std::size_t>(arg, value(arg));
        } else if (arg == "--width") {
            if (options.command == "git") {
                throw usage_error { fmt::format("{} is not an option of `heimdall git`", arg) };
            }
            options.width = number<std::size_t>(arg, value(arg));
        } else if (arg == "--color") {
            const auto when = value(arg);
            if (when == "auto") {
                options.color = heimdall::color_mode::automatic;
            } else if (when == "always") {
                options.color = heimdall::color_mode::always;
            } else if (when == "never") {
                options.color = heimdall::color_mode::never;
            } else {
                throw usage_error { fmt::format("--color takes auto, always or never, not '{}'", when) };
            }
        } else if (arg.starts_with('-')) {
            throw usage_error { fmt::format("unknown option: {}", arg) };
        } else if (options.command == "memory") {
            throw usage_error { "memory takes no arguments" };
        } else if (options.command == "tasks") {
            if (not options.paths.empty()) {
                throw usage_error { "tasks takes one id or prefix" };
            }
            options.paths.emplace_back(arg);
        } else {
            git_only("PATH");
            options.paths.emplace_back(arg);
        }
    }
    return options;
}

/// The plan of norn, with the tokens its tasks spent from the transcripts of their sessions.
[[nodiscard]] auto load_plan(const heimdall::config_t& config) -> nova::expected<heimdall::plan_t, nova::error> {
    auto plan = heimdall::load_plan(config.norn);
    if (plan) {
        heimdall::attribute_usage(*plan, heimdall::session_usage(heimdall::expand_home(config.claude),
                                                                  plan->session_ids()));
    }
    return plan;
}

/// The memory directories, the reads of their transcripts from the scan cache, which is then updated.
[[nodiscard]] auto load_memories(const heimdall::config_t& config) -> std::vector<heimdall::memory_t> {
    const auto cache_path = heimdall::scan_cache_path();
    const auto cached = heimdall::parse_scan_cache(heimdall::read_text(cache_path));
    heimdall::scan_cache_t scanned;
    std::vector<heimdall::memory_t> memories;
    for (const auto& dir : heimdall::memory_dirs(config.memory, heimdall::expand_home(config.claude))) {
        memories.push_back(heimdall::load_memory(dir, cached, scanned));
    }
    if (scanned != cached) {
        std::error_code ec;
        std::filesystem::create_directories(cache_path.parent_path(), ec);
        std::ofstream(cache_path) << heimdall::format_scan_cache(scanned);
    }
    return memories;
}

/// `heimdall tasks`: one task in full when `pattern` is its id, else the open tasks under it.
[[nodiscard]] auto tasks(const heimdall::style_t& style, const heimdall::config_t& config, const std::string& pattern,
                         std::size_t width, bool all) -> int
{
    const auto plan = load_plan(config);
    if (not plan) {
        fmt::println(stderr, "heimdall: norn: {}", plan.error().detail);
        return 1;
    }
    const auto view = heimdall::tasks_view {
        .width = width,
        .repos = heimdall::project_repos(*plan, heimdall::discover(config.roots, config.exclude_from_roots,
                                                                   config.repositories), config.projects),
        .shelved = all,
    };
    if (const auto* task = plan->find(pattern)) {
        fmt::print("{}", heimdall::render_task(style, *plan, *task, view));
        return 0;
    }
    const bool any = std::ranges::any_of(plan->tasks, [&](const heimdall::task_t& task) {
        return task.id.starts_with(pattern + "/");
    });
    if (not pattern.empty() and not any) {
        fmt::println(stderr, "heimdall: no task {} and none under it", pattern);
        return 1;
    }
    fmt::print("{}", heimdall::render_tasks(style, *plan, pattern, view));
    return 0;
}

[[nodiscard]] auto run(const options_t& options) -> int {
    std::error_code ec;
    if (not std::filesystem::exists(heimdall::config_path(), ec)) {
        fmt::println(stderr, "heimdall: warning: no configuration at {}; no repositories are tracked",
                     heimdall::config_path().string());
    }
    auto config = heimdall::load_config(heimdall::config_path());
    if (not config) {
        fmt::println(stderr, "heimdall: {}: {} {}", heimdall::config_path().string(), config.error().code.message(),
                     config.error().detail);
        return ExitUsage;
    }

    const auto style = heimdall::style_t::detect(options.color);
    static constexpr std::size_t MaxWidth = 100;
    static constexpr std::size_t Margin = 4;
    const auto width = options.width.value_or(std::min(MaxWidth, nxs::tty::terminal_width()) - Margin);

    if (options.command == "tasks") {
        return tasks(style, *config, options.paths.empty() ? std::string() : options.paths.front(), width,
                     options.all);
    }
    if (options.command == "memory") {
        fmt::print("{}", heimdall::render_memory(style, load_memories(*config), width));
        return 0;
    }

    const auto started = std::chrono::steady_clock::now();
    const auto repos = heimdall::survey(*config, {
        .fetch = options.fetch,
        .jobs = options.jobs,
        .repositories = options.paths,
        .exclude_from_roots = options.exclude_from_roots,
    });
    const auto now = std::chrono::system_clock::now();
    const auto unix_now = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();

    if (options.command == "git") {
        fmt::print("{}", options.brief ? heimdall::render_brief(style, repos, options.fetch, unix_now)
                                       : heimdall::render_report(style, repos, options.fetch, unix_now));
        const bool attention = std::ranges::any_of(repos, &heimdall::repo_t::attention);
        return options.check and attention ? 1 : 0;
    }

    auto input = heimdall::dashboard_input {
        .width = width,
        .now = now,
        .system = heimdall::read_system(),
        .repos = repos,
        .fetched = options.fetch,
        .repo_limit = options.repos,
        .sections = config->dashboard,
    };
    if (std::ranges::contains(config->dashboard, std::string_view("tasks"))) {
        const auto plan = load_plan(*config);
        input.tasks = plan ? heimdall::tasks_summary(style, *plan) : style("norn: " + plan.error().detail, "dim");
    }
    if (std::ranges::contains(config->dashboard, std::string_view("memory"))) {
        input.memory = heimdall::memory_summary(style, load_memories(*config));
    }
    input.elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - started);
    fmt::print("{}", heimdall::render_dashboard(style, input));
    return 0;
}

} // namespace

auto main(int argc, char* argv[]) -> int {
    const std::vector<std::string_view> args(argv + 1, argv + argc);   // NOLINT: the C interface
    try {
        const auto options = parse_args(args);
        return options ? run(*options) : 0;
    } catch (const usage_error& error) {
        fmt::print(stderr, "heimdall: {}\n\n{}", error.message, Usage);
        return ExitUsage;
    } catch (const std::exception& ex) {
        fmt::println(stderr, "heimdall: {}", ex.what());
        return ExitUsage;
    }
}
