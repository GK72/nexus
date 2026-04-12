#pragma once

#include <libnova/expected.hpp>
#include <libnova/error.hpp>
#include <libnova/data.hpp>

#include <fmt/format.h>

#include <cctype>
#include <cstdint>
#include <string_view>
#include <limits>

namespace btx::details {

/**
 * @brief   Converts a hexadecimal character to its integer value.
 *
 * @param   c Character representing a hex digit (0-9, a-f, A-F).
 * @return  The integer value of the hex digit, or a nova::error if invalid.
 */
inline nova::expected<int, nova::error> hex_to_int(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    return {nova::unexpect, nova::error(fmt::format("Invalid hex character: {}", c))};
}

/**
 * @brief   Accumulates bits and flushes them to a byte vector.
 */
class bit_accumulator {
public:
    /**
     * @brief   Constructs a bit accumulator that appends to the given byte vector.
     *
     * @param   out Reference to the output byte vector.
     */
    explicit bit_accumulator(nova::bytes& out)
        : m_out(out)
    {}

    /**
     * @brief   Adds a bit (0 or 1) to the current byte buffer.
     *
     * @param   bit Integer value 0 or 1.
     */
    void add_bit(int bit) {
        if (bit < 0 or bit > 1) {
            return;
        }

        m_current_byte = static_cast<std::uint8_t>((m_current_byte << 1) | bit);
        m_bits_count++;

        if (m_bits_count == 8) {
            flush();
        }
    }

    /**
     * @brief   Flushes any remaining bits in the buffer, padding with zeros if necessary.
     */
    void flush() {
        if (m_bits_count > 0) {
            if (m_bits_count < 8) {
                m_current_byte = static_cast<std::uint8_t>(m_current_byte << (8 - m_bits_count));
            }
            m_out.push_back(static_cast<std::byte>(m_current_byte));
            m_current_byte = 0;
            m_bits_count = 0;
        }
    }

    /**
     * @brief   Parses a hex token (\xHH) from the input string and adds bits to the accumulator.
     *
     * @param   in  Input string view. Will be advanced by 2 characters.
     *
     * @return  Returns 0 on success, or a nova::error on failure.
     */
    auto add_hex(std::string_view& in) -> nova::expected<int, nova::error> {
        if (in.size() < 2) {
            return {nova::unexpect, nova::error("Unexpected end of input while parsing hex token")};
        }

        char h1 = in[0];
        char h2 = in[1];
        in.remove_prefix(2);

        auto v1 = hex_to_int(h1);
        if (not v1) {
            return {nova::unexpect, v1.error()};
        }

        auto v2 = hex_to_int(h2);
        if (not v2) {
            return {nova::unexpect, v2.error()};
        }

        const int val = (*v1 << 4) | *v2;
        for (int i = 7; i >= 0; --i) {
            add_bit((val >> i) & 1);
        }

        return 0;
    }


    /**
     * @brief   Parses a bit token (\bBB...) from the input string and adds bits to the accumulator.
     *
     * @param   in  Input string view. Will be advanced by at least 8 relevant positions.
     *
     * @return  Returns 0 on success, or a nova::error on failure.
     */
    auto add_bits(std::string_view& in) -> nova::expected<int, nova::error> {
        int positions = 0;
        while (positions < 8 and not in.empty()) {
            const char b = in[0];
            if (b == '0' or b == '1') {
                in.remove_prefix(1);
                add_bit(b - '0');
                positions++;
            } else if (b == '_') {
                in.remove_prefix(1);
                positions++;
            } else if (b == '\'') {
                in.remove_prefix(1);
            } else {
                break;
            }
        }

        if (positions < 8) {
            return {nova::unexpect, nova::error(fmt::format("Incomplete \\b token: expected 8 positions, got {}", positions))};
        }
        return 0;
    }


private:
    nova::bytes& m_out;         ///< Output byte vector.
    std::uint8_t m_current_byte = 0; ///< Current byte being constructed.
    int m_bits_count = 0;       ///< Number of bits currently in m_current_byte.
};



/**
 * @brief   Skips a comment in the input string.
 *
 * @param   in  Input string view. Will be advanced to the next line or end of string.
 *
 * @return  Returns 0 on success, or a nova::error on failure.
 */
inline nova::expected<int, nova::error> skip_comment(std::string_view& in) {
    if (in.size() >= 2 and in[0] == '/' and in[1] == '/') {
        in.remove_prefix(2);
        if (const auto pos = in.find('\n'); pos != std::string_view::npos) {
            in.remove_prefix(pos + 1);
        } else {
            in.remove_prefix(in.size());
        }
        return 0;
    }
    return {nova::unexpect, nova::error("Invalid comment: expected '//'")};
}

} // namespace btx::details
