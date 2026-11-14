/**
 * Part of Nexus Library
 *
 * @brief   Terminal styling and text layout for Heimdall's views.
 *
 * @author      Gábor Krisztián Girhiny
 * @created     2026-11-10
 * @updated     2026-11-10
 * @status      experimental
 * @reviewed    none
 * SPDX-License-Identifier: Unlicense
 */

#include "heimdall/style.hpp"

#include <libnxs/tty.hpp>

#include <fmt/format.h>

#include <algorithm>
#include <array>
#include <cstdlib>
#include <utility>

#include <unistd.h>

namespace heimdall {

namespace {

constexpr auto Palette = std::to_array<std::pair<std::string_view, std::uint32_t>>({
    { "accent", 0x8bd5ca }, { "title", 0xb7bdf8 }, { "text", 0xcad3f5 }, { "dim", 0x6e738d },
    { "faint", 0x494d64 }, { "green", 0xa6da95 }, { "yellow", 0xeed49f }, { "orange", 0xf5a97f },
    { "blue", 0x8aadf4 }, { "red", 0xed8796 }, { "mauve", 0xc6a0f6 }, { "pink", 0xf5bde6 },
});

[[nodiscard]] auto is_lead_byte(char byte) -> bool {
    return (static_cast<unsigned char>(byte) & 0xC0U) != 0x80U;
}

/// The SGR parameters of a foreground colour.
[[nodiscard]] auto color_code(std::uint32_t rgb, bool truecolor) -> std::string {
    const auto red = (rgb >> 16U) & 0xFFU;
    const auto green = (rgb >> 8U) & 0xFFU;
    const auto blue = rgb & 0xFFU;
    if (truecolor) {
        return fmt::format("38;2;{};{};{}", red, green, blue);
    }
    static constexpr auto Cube = std::to_array<std::uint32_t>({ 0, 95, 135, 175, 215, 255 });
    const auto nearest = [](std::uint32_t value) -> std::uint32_t {
        const auto* best = std::ranges::min_element(Cube, {}, [&](std::uint32_t level) {
            return level > value ? level - value : value - level;
        });
        return static_cast<std::uint32_t>(best - Cube.begin());
    };
    return fmt::format("38;5;{}", 16 + 36 * nearest(red) + 6 * nearest(green) + nearest(blue));
}

[[nodiscard]] auto env(const char* name) -> std::string_view {
    const char* value = std::getenv(name);  // NOLINT(concurrency-mt-unsafe): read-only
    return value != nullptr ? value : "";
}

} // namespace

auto style_t::detect(color_mode mode) -> style_t {
    const auto colorterm = env("COLORTERM");
    const bool truecolor = colorterm == "truecolor" or colorterm == "24bit";
    switch (mode) {
        case color_mode::always: return { .enabled = true, .truecolor = truecolor };
        case color_mode::never:  return { .enabled = false, .truecolor = false };
        case color_mode::automatic: break;
    }
    const bool enabled = ::isatty(STDOUT_FILENO) != 0 and std::getenv("NO_COLOR") == nullptr  // NOLINT
                         and env("TERM") != "dumb";
    return { .enabled = enabled, .truecolor = truecolor };
}

auto style_t::operator()(std::string_view text, std::string_view color, bool bold) const -> std::string {
    if (not enabled or text.empty() or (color.empty() and not bold)) {
        return std::string(text);
    }
    std::string codes = bold ? "1" : "";
    if (const auto* entry = std::ranges::find(Palette, color, &std::pair<std::string_view, std::uint32_t>::first);
        entry != Palette.end())
    {
        codes += (codes.empty() ? "" : ";") + color_code(entry->second, truecolor);
    }
    return fmt::format("\x1b[{}m{}\x1b[0m", codes, text);
}

auto visible_width(std::string_view text) -> std::size_t {
    return nxs::tty::visible_width(std::string(text));
}

auto pad(std::string_view text, std::size_t width) -> std::string {
    const auto used = visible_width(text);
    return std::string(text) + std::string(used < width ? width - used : 0, ' ');
}

auto ellipsize(std::string_view text, std::size_t width) -> std::string {
    if (visible_width(text) <= width) {
        return std::string(text);
    }
    std::string ret;
    std::size_t kept = 0;
    for (const char byte : text) {
        if (is_lead_byte(byte) and ++kept > width - 1) {
            break;
        }
        ret += byte;
    }
    return width > 0 ? ret + "…" : ret;
}

auto spread(const std::string& left, const std::string& right, std::size_t width) -> std::string {
    const auto used = visible_width(left) + visible_width(right);
    return left + std::string(std::max<std::size_t>(2, width > used ? width - used : 0), ' ') + right;
}

auto rule(const style_t& style, std::string_view title, std::size_t width, const std::string& info) -> std::string {
    const auto left = style("──", "faint") + " " + style(title, "title", true) + " ";
    const auto right = info.empty() ? std::string() : " " + info;
    const auto used = visible_width(left) + visible_width(right);
    std::string fill;
    for (auto count = std::max<std::size_t>(2, width > used ? width - used : 0); count > 0; --count) {
        fill += "─";
    }
    return left + style(fill, "faint") + right;
}

auto wrap(std::string_view text, std::size_t width) -> std::vector<std::string> {
    std::vector<std::string> lines;
    while (true) {
        const auto end = text.find('\n');
        const auto paragraph = text.substr(0, end);
        std::string line;
        std::size_t used = 0;
        std::size_t start = 0;
        while (start < paragraph.size()) {
            const auto stop = std::min(paragraph.find(' ', start), paragraph.size());
            const auto word = paragraph.substr(start, stop - start);
            start = stop + 1;
            if (word.empty()) {
                continue;
            }
            const auto size = visible_width(word);
            if (used > 0 and used + 1 + size > width) {
                lines.push_back(std::move(line));
                line.clear();
                used = 0;
            }
            line += (used > 0 ? " " : "") + std::string(word);
            used += (used > 0 ? 1 : 0) + size;
        }
        lines.push_back(std::move(line));
        if (end == std::string_view::npos) {
            break;
        }
        text.remove_prefix(end + 1);
    }
    return lines;
}

auto join(std::span<const std::string> items, std::string_view separator) -> std::string {
    std::string ret;
    for (const auto& item : items) {
        if (item.empty()) {
            continue;
        }
        ret += (ret.empty() ? "" : std::string(separator)) + item;
    }
    return ret;
}

auto ago(std::int64_t seconds) -> std::string {
    static constexpr auto Units = std::to_array<std::pair<std::string_view, std::int64_t>>({
        { "y", 31'536'000 }, { "mo", 2'592'000 }, { "w", 604'800 }, { "d", 86'400 }, { "h", 3'600 }, { "m", 60 },
    });
    seconds = std::max<std::int64_t>(0, seconds);
    for (const auto& [unit, size] : Units) {
        const auto threshold = (unit == "mo" or unit == "w") ? 2 * size : size;
        if (seconds >= threshold) {
            return fmt::format("{}{}", seconds / size, unit);
        }
    }
    return fmt::format("{}s", seconds);
}

auto home_path(std::string_view path) -> std::string {
    const auto home = env("HOME");
    if (not home.empty() and path.starts_with(home)
        and (path.size() == home.size() or path[home.size()] == '/'))
    {
        return "~" + std::string(path.substr(home.size()));
    }
    return std::string(path);
}

auto expand_home(std::string_view path) -> std::string {
    if (path == "~" or path.starts_with("~/")) {
        return std::string(env("HOME")) + std::string(path.substr(1));
    }
    return std::string(path);
}

} // namespace heimdall
