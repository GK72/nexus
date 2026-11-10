/**
 * Part of Nexus Library
 *
 * @brief   Terminal styling and text layout for Heimdall's views.
 *
 * Colours are names of a fixed palette, written as truecolor when the
 * terminal says so (`COLORTERM`), else as the nearest of the 256-colour cube.
 * Widths count UTF-8 characters, not bytes, and skip ANSI escapes.
 *
 * @author      Gábor Krisztián Girhiny
 * @created     2026-11-10
 * @updated     2026-11-10
 * @status      experimental
 * @reviewed    none
 * SPDX-License-Identifier: Unlicense
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>

namespace heimdall {

enum class color_mode : std::uint8_t { automatic, always, never };

/**
 * @brief   Paints text with the palette, or leaves it plain when disabled.
 *
 * Palette names: accent, title, text, dim, faint, green, yellow, orange, blue,
 * red, mauve, pink.
 */
struct style_t {
    bool enabled = false;
    bool truecolor = false;

    /**
     * @brief   Enabled on a terminal, unless `NO_COLOR` is set or `TERM` is
     *          `dumb`; `mode` overrides.
     */
    [[nodiscard]] static auto detect(color_mode mode) -> style_t;

    [[nodiscard]] auto operator()(std::string_view text, std::string_view color = {}, bool bold = false) const
        -> std::string;
};

/**
 * @brief   Columns `text` takes on a terminal.
 */
[[nodiscard]] auto visible_width(std::string_view text) -> std::size_t;

/**
 * @brief   `text` filled with spaces to `width` columns.
 */
[[nodiscard]] auto pad(std::string_view text, std::size_t width) -> std::string;

/**
 * @brief   Plain `text` cut to `width` columns, ending with `…` when cut.
 */
[[nodiscard]] auto ellipsize(std::string_view text, std::size_t width) -> std::string;

/**
 * @brief   The items with `separator` between them.
 */
[[nodiscard]] auto join(std::span<const std::string> items, std::string_view separator) -> std::string;

/**
 * @brief   A compact age: 45s, 12m, 3h, 5d, 7w, 4mo, 2y.
 */
[[nodiscard]] auto ago(std::int64_t seconds) -> std::string;

/**
 * @brief   `path` with the home directory written as `~`.
 */
[[nodiscard]] auto home_path(std::string_view path) -> std::string;

/**
 * @brief   `path` with a leading `~` expanded to the home directory.
 */
[[nodiscard]] auto expand_home(std::string_view path) -> std::string;

} // namespace heimdall
