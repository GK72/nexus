#include <libbtx/btx.hpp>
#include <libbtx/details/btx_helpers.hpp>

#include <libnova/data.hpp>
#include <libnova/error.hpp>
#include <libnova/expected.hpp>

#include <fmt/format.h>

#include <cctype>

namespace btx {

auto to_binary(std::istream& in, std::ostream& out) -> nova::expected<int, nova::error> {
    details::bit_accumulator accumulator;

    for (char ch = '\0'; in.get(ch); ) {
        if (ch == '/') {
            if (const auto res = details::skip_comment(in); not res) {
                return { nova::unexpect, res.error() };
            }
        } else if (std::isspace(static_cast<unsigned char>(ch)) or ch == '\'' or ch == '_') {
            // Ignore whitespace and separators
        } else if (ch == '\\') {
            const char type = static_cast<char>(in.peek());
            if (type == 'x') {
                in.get();
                if (const auto res = accumulator.add_hex_from_stream(in, out); not res) {
                    return { nova::unexpect, res.error() };
                }
            } else if (type == 'b') {
                in.get();
                if (const auto res = accumulator.add_bits_from_stream(in, out); not res) {
                    return { nova::unexpect, res.error() };
                }
            } else {
                return { nova::unexpect, nova::error(fmt::format("Unexpected character '\\' followed by '{}'", type)) };
            }
        } else {
            return { nova::unexpect, nova::error(fmt::format("Unexpected character: '{}'", ch)) };
        }
    }

    accumulator.flush(out);
    return 0;
}

auto from_binary(nova::data_view in, std::ostream& out, const config& config) -> nova::expected<int, nova::error> {
    if (in.empty()) {
        return 0;
    }

    constexpr std::size_t BytesPerLine = 8;

    for (std::size_t i = 0; i < in.size(); ++i) {
        const auto byte = in.as_number<std::uint8_t>(i);
        out << fmt::format("\\x{:02X}", byte);

        if (config.format_output) {
            if ((i + 1) % BytesPerLine == 0) {
                out << "\n";
            }
        }
    }
    return 0;
}

} // namespace btx
