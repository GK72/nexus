#include <libbtx/btx.hpp>
#include <libbtx/details/btx_helpers.hpp>

#include <libnova/data.hpp>
#include <libnova/error.hpp>
#include <libnova/expected.hpp>

#include <fmt/format.h>

#include <cctype>

namespace btx {

auto to_binary(std::string_view in) -> nova::expected<nova::bytes, nova::error> {
    nova::bytes              result;
    details::bit_accumulator accumulator(result);

    while (not in.empty()) {
        char ch = in[0];
        if (ch == '/') {
            if (const auto res = details::skip_comment(in); not res) {
                return {nova::unexpect, res.error()};
            }
        } else if (std::isspace(static_cast<unsigned char>(ch)) or ch == '\'' or ch == '_') {
            in.remove_prefix(1);
        } else if (ch == '\\') {
            if (in.size() < 2) {
                return {nova::unexpect, nova::error("Unexpected end of input after '\\'")};
            }
            const char type = in[1];
            in.remove_prefix(2);
            if (type == 'x') {
                if (const auto res = accumulator.add_hex(in); not res) {
                    return {nova::unexpect, res.error()};
                }
            } else if (type == 'b') {
                if (const auto res = accumulator.add_bits(in); not res) {
                    return {nova::unexpect, res.error()};
                }
            } else {
                return {nova::unexpect, nova::error(fmt::format("Unexpected character '\\' followed by '{}'", type))};
            }
        } else {
            return {nova::unexpect, nova::error(fmt::format("Unexpected character: '{}'", ch))};
        }
    }

    accumulator.flush();
    return result;
}

auto from_binary(nova::data_view in, const config& config) -> nova::expected<nova::bytes, nova::error> {
    if (in.empty()) {
        return nova::bytes{ };
    }

    constexpr std::size_t BytesPerLine = 8;
    std::string           result;

    for (std::size_t i = 0; i < in.size(); ++i) {
        const auto byte = in.as_number<std::uint8_t>(i);
        result += fmt::format("\\x{:02X}", byte);

        if (config.format_output) {
            if ((i + 1) % BytesPerLine == 0) {
                result += "\n";
            }
        }
    }
    return nova::data_view(result).to_vec();
}

} // namespace btx
