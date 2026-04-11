/**
 * Part of Nexus project.
 *
 * Binary Text processing.
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

#include <iostream>

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
 * @brief   Converts BTX text from an input stream to binary in an output stream.
 *
 * @param in    Input stream containing BTX text.
 * @param out   Output stream where binary data will be written.
 *
 * @return  Returns 0 on success, or a nova::error on failure.
 */
nova::expected<int, nova::error> to_binary(std::istream& in, std::ostream& out);

/**
 * @brief   Converts binary data from a data_view to BTX text in an output stream.
 *
 * @param in        Non-owning view of the binary data.
 * @param out       Output stream where BTX text will be written.
 * @param config    Optional configuration for the conversion.
 *
 * @return  Returns 0 on success, or a nova::error on failure.
 */
nova::expected<int, nova::error> from_binary(nova::data_view in, std::ostream& out, const config& config = {});

} // namespace btx
