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

#include "heimdall/dashboard.hpp"

#include "heimdall/report.hpp"

#include <fmt/format.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <ctime>
#include <fstream>
#include <set>
#include <utility>

#include <sys/statvfs.h>
#include <sys/utsname.h>
#include <unistd.h>

namespace heimdall {

namespace {

constexpr std::size_t MeterWidth = 10;

[[nodiscard]] auto os_name() -> std::string {
    std::ifstream file("/etc/os-release");
    for (std::string line; std::getline(file, line);) {
        if (line.starts_with("NAME=")) {
            auto name = line.substr(5);
            std::erase(name, '"');
            return name;
        }
    }
    utsname info {};
    return ::uname(&info) == 0 ? info.sysname : "unknown";
}

[[nodiscard]] auto uptime_seconds() -> std::int64_t {
    std::ifstream file("/proc/uptime");
    double seconds = 0;
    file >> seconds;
    return static_cast<std::int64_t>(seconds);
}

[[nodiscard]] auto uptime_text(std::int64_t seconds) -> std::string {
    const auto days = seconds / 86'400;
    const auto hours = seconds % 86'400 / 3'600;
    const auto minutes = seconds % 3'600 / 60;
    return days > 0 ? fmt::format("{}d {}h", days, hours) : fmt::format("{}h {}m", hours, minutes);
}

[[nodiscard]] auto strftime_text(std::chrono::system_clock::time_point when, const char* format) -> std::string {
    const auto time = std::chrono::system_clock::to_time_t(when);
    std::tm local {};
    ::localtime_r(&time, &local);
    std::array<char, 64> buffer {};
    const auto size = std::strftime(buffer.data(), buffer.size(), format, &local);
    return { buffer.data(), size };
}

[[nodiscard]] auto meter(const style_t& style, double ratio) -> std::string {
    const auto color = ratio < 0.6 ? "green" : ratio < 0.85 ? "yellow" : "red";
    const auto filled = std::min(MeterWidth, static_cast<std::size_t>(std::lround(ratio * MeterWidth)));
    std::string full;
    std::string empty;
    for (std::size_t index = 0; index < MeterWidth; ++index) {
        (index < filled ? full : empty) += index < filled ? "▰" : "▱";
    }
    return style(full, color) + style(empty, "faint");
}

/// `left` and `right` on one line of `width` columns.
[[nodiscard]] auto spread(const std::string& left, const std::string& right, std::size_t width) -> std::string {
    const auto used = visible_width(left) + visible_width(right);
    return left + std::string(std::max<std::size_t>(2, width > used ? width - used : 0), ' ') + right;
}

/// A section's rule: `── title ───── info`.
[[nodiscard]] auto rule(const style_t& style, std::string_view title, std::size_t width, const std::string& info = {})
    -> std::string
{
    const auto left = style("──", "faint") + " " + style(title, "title", true) + " ";
    const auto right = info.empty() ? std::string() : " " + info;
    const auto used = visible_width(left) + visible_width(right);
    std::string fill;
    for (auto count = std::max<std::size_t>(2, width > used ? width - used : 0); count > 0; --count) {
        fill += "─";
    }
    return left + style(fill, "faint") + right;
}

[[nodiscard]] auto header(const style_t& style, const dashboard_input& input) -> std::vector<std::string> {
    const auto date = style(strftime_text(input.now, "%A, %-d %B %Y"), "text");
    const auto time = style(strftime_text(input.now, "%H:%M"), "text", true)
                    + style(strftime_text(input.now, " · week %V"), "dim");
    return {
        spread(style("Dashboard", "title", true), date, input.width),
        spread(style("weather: not yet available", "dim"), time, input.width),
    };
}

[[nodiscard]] auto system_lines(const style_t& style, const system_t& system) -> std::vector<std::string> {
    const auto separator = style(" · ", "faint");
    std::vector<std::string> facts {
        style(system.host, "accent"),
        style(system.os, "text"),
        style("kernel ", "dim") + style(system.kernel, "text"),
        style("up ", "dim") + style(uptime_text(system.uptime), "text"),
    };
    std::vector<std::string> disks;
    for (const auto& disk : system.disks) {
        disks.push_back(style(disk.label, "dim") + " " + meter(style, disk.ratio)
                        + style(fmt::format(" {}%", std::lround(disk.ratio * 100)), "text"));
    }
    return { join(facts, separator), style("Disks: ", "dim") + join(disks, "   ") };
}

[[nodiscard]] auto repo_lines(const style_t& style, const dashboard_input& input) -> std::vector<std::string> {
    auto rows = brief_rows(style, input.repos);
    if (rows.empty()) {
        return { style("✓ ", "green") + style("every repository is clean and pushed", "dim") };
    }
    if (rows.size() > input.repo_limit) {
        const auto more = rows.size() - input.repo_limit;
        rows.resize(input.repo_limit);
        rows.push_back(style(fmt::format("… {} more", more), "dim") + style("  heimdall git", "faint"));
    }
    return rows;
}

} // namespace

auto read_system() -> system_t {
    system_t system { .os = os_name(), .uptime = uptime_seconds() };

    std::array<char, 256> host {};
    if (::gethostname(host.data(), host.size() - 1) == 0) {
        system.host = host.data();
    }
    utsname info {};
    if (::uname(&info) == 0) {
        const std::string release = info.release;
        system.kernel = release.substr(0, release.find('-'));
    }

    std::set<std::pair<unsigned long, unsigned long>> seen;   // btrfs subvolumes look like mounts of their own
    const auto add = [&](const std::string& label, const std::string& path) {
        struct statvfs stats {};
        if (::statvfs(path.c_str(), &stats) != 0 or stats.f_blocks == 0) {
            return;
        }
        const auto total = stats.f_blocks * stats.f_frsize;
        const auto used = (stats.f_blocks - stats.f_bfree) * stats.f_frsize;
        if (not seen.emplace(total, used).second) {
            return;
        }
        system.disks.push_back({ .label = label, .ratio = static_cast<double>(used) / static_cast<double>(total) });
    };
    add("/", "/");
    add("~", expand_home("~"));
    add("/mnt/data", "/mnt/data");
    return system;
}

auto render_dashboard(const style_t& style, const dashboard_input& input) -> std::string {
    const auto now = std::chrono::duration_cast<std::chrono::seconds>(input.now.time_since_epoch()).count();
    const auto attention = std::ranges::count_if(input.repos, &repo_t::attention);
    auto info = style(fmt::format("{}", input.repos.size()), "text") + style(" repos", "dim");
    if (attention > 0) {
        info += style(" · ", "faint") + style(fmt::format("{} need attention", attention), "yellow");
    }
    info += style(" · ", "faint") + style(fetch_info(input.repos, input.fetched, now), "dim");

    std::vector<std::string> out { "" };
    std::ranges::move(header(style, input), std::back_inserter(out));
    out.emplace_back();
    out.push_back(rule(style, "system", input.width));
    std::ranges::move(system_lines(style, input.system), std::back_inserter(out));
    out.emplace_back();
    out.push_back(rule(style, "repos", input.width, info));
    std::ranges::move(repo_lines(style, input), std::back_inserter(out));
    const auto elapsed = fmt::format("{} ms", input.elapsed.count());
    out.push_back(std::string(input.width > elapsed.size() ? input.width - elapsed.size() : 0, ' ')
                  + style(elapsed, "faint"));

    std::string text;
    for (const auto& line : out) {
        text += (line.empty() ? "" : "  " + line) + "\n";
    }
    return text;
}

} // namespace heimdall
