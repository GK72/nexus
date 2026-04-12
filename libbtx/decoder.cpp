#include <libbtx/decoder.hpp>
#include <fmt/format.h>
#include <iomanip>
#include <map>
#include <sstream>

namespace btx {

namespace {

constexpr std::size_t BitsPerByte = 8;

/**
 * @brief Resolves the length of a field in bits.
 */
auto get_field_length_bits(const descriptor::field& field, const std::map<std::string, uint64_t>& symbol_table) -> nova::expected<std::size_t, nova::error> {
    std::size_t length_val = 0;
    if (std::holds_alternative<std::size_t>(field.length)) {
        length_val = std::get<std::size_t>(field.length);
    } else {
        const auto& ref_name = std::get<std::string>(field.length);
        if (symbol_table.contains(ref_name)) {
            length_val = symbol_table.at(ref_name);
        } else {
            return nova::unexpected(nova::error("Length reference not found: " + ref_name));
        }
    }
    return (field.len_type == descriptor::length_type::byte) ? length_val * BitsPerByte : length_val;
}

/**
 * @brief Decodes a single field and updates the bit position and symbol table.
 */
auto decode_field(const descriptor::field& field_item, nova::data_view data, std::size_t& bit_pos, std::map<std::string, uint64_t>& symbol_table) -> nova::expected<field_value, nova::error> {
    const auto length_bits_res = get_field_length_bits(field_item, symbol_table);
    if (not length_bits_res) {
        return nova::unexpected(length_bits_res.error());
    }
    const std::size_t length_bits = *length_bits_res;

    field_value fv;
    fv.name = field_item.name;

    try {
        if (field_item.type == descriptor::field_type::unsigned_integer) {
            uint64_t val = data.as_number_bit_packed<uint64_t>(bit_pos, length_bits);
            fv.value = val;
            symbol_table[field_item.name] = val;
            bit_pos += length_bits;
        } else if (field_item.type == descriptor::field_type::boolean) {
            bool val = data.as_number_bit_packed<uint64_t>(bit_pos, 1) != 0;
            fv.value = val;
            symbol_table[field_item.name] = val ? 1 : 0;
            bit_pos += 1;
        } else if (field_item.type == descriptor::field_type::string) {
            if (bit_pos % BitsPerByte != 0) {
                return nova::unexpected(nova::error("String field '" + field_item.name + "' must be byte-aligned"));
            }
            std::string val(data.as_string(bit_pos / BitsPerByte, length_bits / BitsPerByte));
            fv.value = val;
            bit_pos += length_bits;
        }
    } catch (const std::exception& e) {
        return nova::unexpected(nova::error("Error decoding field '" + field_item.name + "': " + e.what()));
    }

    return fv;
}

/**
 * @brief Formats a single field to the output string and updates the bit position and symbol table.
 */
auto format_field(const descriptor::field& field_item, nova::data_view data, std::size_t& bit_pos, std::map<std::string, uint64_t>& symbol_table, std::string& out) -> nova::expected<int, nova::error> {
    const auto length_bits_res = get_field_length_bits(field_item, symbol_table);
    if (not length_bits_res) {
        return nova::unexpected(length_bits_res.error());
    }
    std::size_t length_bits = *length_bits_res;

    try {
        if (field_item.type == descriptor::field_type::unsigned_integer || field_item.type == descriptor::field_type::boolean) {
            uint64_t val = 0;
            if (field_item.type == descriptor::field_type::unsigned_integer) {
                val = data.as_number_bit_packed<uint64_t>(bit_pos, length_bits);
                symbol_table[field_item.name] = val;
            } else {
                val = data.as_number_bit_packed<uint64_t>(bit_pos, 1) != 0;
                symbol_table[field_item.name] = val;
                length_bits = 1;
            }

            std::string btx_str;
            if (length_bits % BitsPerByte == 0 && bit_pos % BitsPerByte == 0) {
                for (std::size_t i = 0; i < length_bits / BitsPerByte; ++i) {
                    btx_str += fmt::format("\\x{:02X}", static_cast<uint32_t>(data.span()[bit_pos / BitsPerByte + i]));
                }
            } else {
                btx_str += "\\b";
                for (std::size_t i = 0; i < length_bits; ++i) {
                    bool bit = data.as_number_bit_packed<uint64_t>(bit_pos + i, 1) != 0;
                    btx_str += (bit ? '1' : '0');
                }
            }
            out += fmt::format("{:<40} // {}: {} (0x{:x})\n", btx_str, field_item.name, val, val);
            bit_pos += length_bits;
        } else if (field_item.type == descriptor::field_type::string) {
            if (bit_pos % BitsPerByte != 0) {
                return nova::unexpected(nova::error("String field '" + field_item.name + "' must be byte-aligned"));
            }
            std::string val(data.as_string(bit_pos / BitsPerByte, length_bits / BitsPerByte));
            for (std::size_t i = 0; i < length_bits / BitsPerByte; ++i) {
                out += fmt::format("\\x{:02X}", static_cast<uint32_t>(data.span()[bit_pos / BitsPerByte + i]));
                if ((i + 1) % BitsPerByte == 0 || (i + 1) == length_bits / BitsPerByte) {
                    if ((i + 1) == length_bits / BitsPerByte) {
                        std::size_t current_line_bytes = (i % BitsPerByte) + 1;
                        if (current_line_bytes <= 8) {
                            out += std::string((10 - current_line_bytes) * 4, ' ');
                        }
                        out += fmt::format(" // {}: \"{}\"", field_item.name, val);
                    }
                    out += "\n";
                }
            }
            bit_pos += length_bits;
        }
    } catch (const std::exception& e) {
        return nova::unexpected(nova::error("Error formatting field '" + field_item.name + "': " + e.what()));
    }

    return 0;
}

} // namespace

auto decode(const descriptor& desc, nova::data_view data) -> nova::expected<message_data, nova::error> {
    message_data result;
    result.name = desc.message.name;

    std::size_t bit_pos = 0;
    std::map<std::string, std::uint64_t> symbol_table;

    for (const auto& field_item : desc.message.fields) {
        auto fv_res = decode_field(field_item, data, bit_pos, symbol_table);
        if (not fv_res) {
            return nova::unexpected(fv_res.error());
        }
        result.fields.push_back(*fv_res);
    }

    return result;
}

auto format(const descriptor& desc, nova::data_view data) -> nova::expected<nova::bytes, nova::error> {
    std::size_t bit_pos = 0;
    std::map<std::string, uint64_t> symbol_table;
    std::string result;

    result += fmt::format("// {}\n", desc.message.name);

    for (const auto& field_item : desc.message.fields) {
        auto res = format_field(field_item, data, bit_pos, symbol_table, result);
        if (not res) {
            return nova::unexpected(res.error());
        }
    }

    return nova::data_view(result).to_vec();
}

} // namespace btx
