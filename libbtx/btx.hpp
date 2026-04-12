/**
 * Part of BTX Toolset.
 *
 * Binary Text processing.
 *
 * @author  Gábor Krisztián Girhiny and Junie
 * @date    2026-04-11
 *
 * To convert BTX to a raw binary file:
 * 1. Remove all comments (everything from `//` to the end of the line).
 * 2. Remove all whitespace and the `'` separator.
 * 3. Process `\xHH` and `\b` tokens. Note that `\b` tokens must have exactly 8
 *    positions (including bits and `_` placeholders).
 * 4. All tokens are concatenated. Every 8 bits form one byte.
 */

#pragma once

#include <libnova/data.hpp>
#include <libnova/error.hpp>
#include <libnova/expected.hpp>

#include <string_view>

namespace btx {

/**
 * @brief   Enum defining the conversion direction.
 */
enum class conversion_mode {
    to_binary,   ///< Convert BTX text to binary.
    from_binary  ///< Convert binary to BTX text.
};

/**
 * @brief   Configuration settings for BTX conversion.
 */
struct config {
    bool format_output = true;    ///< Whether to format the output with spaces and newlines.
};

/**
 * @brief   Converts BTX text to binary.
 *
 * @param in    Input string view containing BTX text.
 *
 * @return  Returns the binary data on success, or a nova::error on failure.
 */
nova::expected<nova::bytes, nova::error> to_binary(std::string_view in);

/**
 * @brief   Converts binary data from a data_view to BTX text.
 *
 * @param in        Non-owning view of the binary data.
 * @param config    Optional configuration for the conversion.
 *
 * @return  Returns the BTX text on success, or a nova::error on failure.
 */
nova::expected<nova::bytes, nova::error> from_binary(nova::data_view in, const config& config = {});

} // namespace btx
