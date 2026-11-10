/**
 * Part of Nexus Library
 *
 * @brief   The heimdall CLI: the dashboard, and the views of its parts.
 *
 * @author      Gábor Krisztián Girhiny
 * @created     2026-11-10
 * @updated     2026-11-10
 * @status      experimental
 * @reviewed    none
 * SPDX-License-Identifier: Unlicense
 */

#include "heimdall/config.hpp"
#include "heimdall/dashboard.hpp"
#include "heimdall/git.hpp"
#include "heimdall/heimdall.hpp"
#include "heimdall/report.hpp"

#include <libnxs/tty.hpp>

#include <fmt/format.h>

#include <charconv>
#include <chrono>
#include <cstdio>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace {

constexpr std::string_view Usage = R"(usage: heimdall [-f] [--repos N] [--width N] [--color WHEN]
       heimdall git [-f] [-b] [--check] [--exclude-from-roots GLOB]... [-j N] [--color WHEN] [PATH...]

The dashboard, or with `git`, the report of the tracked repositories.

  -f, --fetch                  fetch every repository first
  --repos N                    dashboard: repositories listed at most (default 8)
  --width N                    dashboard: columns (default: the terminal's, at most 100)
  -b, --brief                  git: one line per repository that needs attention
  --check                      git: exit status 1 if anything needs attention
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
    int jobs = 8;
    std::size_t repos = 8;
    std::optional<std::size_t> width;
    heimdall::color_mode color = heimdall::color_mode::automatic;
    std::vector<std::string> paths;
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
        if (options.command != "git") {
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
        if (options.command != "git") {
            throw usage_error { fmt::format("{} is an option of `heimdall git`", option) };
        }
    };
    const auto dashboard_only = [&](std::string_view option) {
        if (not options.command.empty()) {
            throw usage_error { fmt::format("{} is an option of the dashboard", option) };
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
            options.fetch = true;
        } else if (arg == "-b" or arg == "--brief") {
            git_only(arg);
            options.brief = true;
        } else if (arg == "--check") {
            git_only(arg);
            options.check = true;
        } else if (arg == "--exclude-from-roots") {
            git_only(arg);
            options.exclude_from_roots.emplace_back(value(arg));
        } else if (arg == "-j" or arg == "--jobs") {
            options.jobs = number<int>(arg, value(arg));
        } else if (arg == "--repos") {
            dashboard_only(arg);
            options.repos = number<std::size_t>(arg, value(arg));
        } else if (arg == "--width") {
            dashboard_only(arg);
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
        } else {
            git_only("PATH");
            options.paths.emplace_back(arg);
        }
    }
    return options;
}

[[nodiscard]] auto run(const options_t& options) -> int {
    auto config = heimdall::load_config(heimdall::config_path());
    if (not config) {
        fmt::println(stderr, "heimdall: {}: {} {}", heimdall::config_path().string(), config.error().code.message(),
                     config.error().detail);
        return ExitUsage;
    }

    const auto style = heimdall::style_t::detect(options.color);
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

    static constexpr std::size_t MaxWidth = 100;
    static constexpr std::size_t Margin = 4;
    auto input = heimdall::dashboard_input {
        .width = options.width.value_or(std::min(MaxWidth, nxs::tty::terminal_width()) - Margin),
        .now = now,
        .system = heimdall::read_system(),
        .repos = repos,
        .fetched = options.fetch,
        .repo_limit = options.repos,
    };
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
