#pragma once

#include <libnova/expected.hpp>
#include <libnova/error.hpp>

#include <fmt/format.h>

#include <cctype>
#include <cstdint>
#include <iostream>
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
 * @brief   Accumulates bits and flushes them to an output stream as bytes.
 */
class bit_accumulator {
public:
    /**
     * @brief   Adds a bit (0 or 1) to the current byte buffer.
     *
     * @param   bit Integer value 0 or 1.
     * @param   out Output stream where complete bytes are written.
     */
    void add_bit(int bit, std::ostream& out) {
        if (bit < 0 || bit > 1) {
            return;
        }

        m_current_byte = static_cast<uint8_t>((m_current_byte << 1) | bit);
        m_bits_count++;

        if (m_bits_count == 8) {
            flush(out);
        }
    }

    /**
     * @brief   Flushes any remaining bits in the buffer, padding with zeros if necessary.
     *
     * @param   out Output stream where the byte is written.
     */
    void flush(std::ostream& out) {
        if (m_bits_count > 0) {
            if (m_bits_count < 8) {
                m_current_byte = static_cast<uint8_t>(m_current_byte << (8 - m_bits_count));
            }
            out.put(static_cast<char>(m_current_byte));
            m_current_byte = 0;
            m_bits_count = 0;
        }
    }

    /**
     * @brief   Parses a hex token (\xHH) from the input stream and adds bits to the accumulator.
     *
     * @param   in  Input stream.
     * @param   out Output stream (for flushed bytes).
     *
     * @return  Returns 0 on success, or a nova::error on failure.
     */
    auto add_hex_from_stream(std::istream& in, std::ostream& out) -> nova::expected<int, nova::error> {
        char h1 = '\0';
        char h2 = '\0';

        if (not (in.get(h1) and in.get(h2))) {
            return {nova::unexpect, nova::error("Unexpected end of input while parsing hex token")};
        }

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
            add_bit((val >> i) & 1, out);
        }

        return 0;
    }


    /**
     * @brief   Parses a bit token (\bBB...) from the input stream and adds bits to the accumulator.
     *
     * @param   in  Input stream.
     * @param   out Output stream (for flushed bytes).
     *
     * @return  Returns 0 on success, or a nova::error on failure.
     */
    auto add_bits_from_stream(std::istream& in, std::ostream& out) -> nova::expected<int, nova::error> {
        int positions = 0;
        while (positions < 8) {
            const int b_int = in.peek();
            if (b_int == std::char_traits<char>::to_int_type(EOF)) {
                break;
            }
            const char b = static_cast<char>(b_int);
            if (b == '0' or b == '1') {
                in.get();
                add_bit(b - '0', out);
                positions++;
            } else if (b == '_') {
                in.get();
                positions++;
            } else if (b == '\'') {
                in.get();
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
    uint8_t m_current_byte = 0; ///< Current byte being constructed.
    int m_bits_count = 0;       ///< Number of bits currently in m_current_byte.
};



/**
 * @brief   Skips a comment in the input stream.
 *
 * @param   in  Input stream.
 *
 * @return  Returns 0 on success, or a nova::error on failure.
 */
inline nova::expected<int, nova::error> skip_comment(std::istream& in) {
    if (in.peek() == '/') {
        in.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        return 0;
    }
    return {nova::unexpect, nova::error("Invalid comment: expected '//'")};
}

} // namespace btx::details
