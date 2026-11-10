/**
 * Part of BTX Toolset
 *
 * @brief   The errors of BTX conversion.
 *
 * @author      Gábor Krisztián Girhiny
 * @created     2026-11-10
 * @updated     2026-11-10
 * @status      experimental
 * @reviewed    none
 * SPDX-License-Identifier: Unlicense
 */

#pragma once

#include <libnova/error.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace btx {

enum class error_e : std::uint8_t {
    unexpected_end = 1,         ///< The input ends inside a token
    unexpected_character,       ///< A character that starts no token
    invalid_hex,                ///< Not a hex digit in a `\x` token
    incomplete_bits,            ///< A `\b` token without 8 positions
    invalid_comment,            ///< A `/` not followed by another
};

using nova::make_error_code;

} // namespace btx

template <>
struct nova::error_code_traits<btx::error_e> {
    static constexpr auto name = "btx";
    static constexpr auto messages = std::to_array<std::string_view>({
        "Unexpected end of input",
        "Unexpected character",
        "Invalid hex character",
        "Incomplete \\b token",
        "Invalid comment",
    });

    static_assert(messages.size() == static_cast<std::size_t>(btx::error_e::invalid_comment));
};
