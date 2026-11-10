/**
 * Part of BTX Toolset
 *
 * @brief   The errors of Lexy.
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

namespace lexy {

enum class error_e : std::uint8_t {
    invalid_descriptor = 1,     ///< A YAML descriptor that does not describe a message
    syntax,                     ///< A `.lxy` descriptor that does not parse
    encode,                     ///< Values that do not fit the descriptor
    decode,                     ///< Binary data that does not fit the descriptor
};

using nova::make_error_code;

} // namespace lexy

template <>
struct nova::error_code_traits<lexy::error_e> {
    static constexpr auto name = "lexy";
    static constexpr auto messages = std::to_array<std::string_view>({
        "Invalid descriptor",
        "Syntax error",
        "Encoding failed",
        "Decoding failed",
    });

    static_assert(messages.size() == static_cast<std::size_t>(lexy::error_e::decode));
};
