#include <libbtx/generator.hpp>
#include <libnova/random.hpp>
#include <yaml-cpp/yaml.h>
#include <map>
#include <set>

namespace btx::generator {

namespace {

constexpr std::size_t BitsPerByte = 8;

/**
 * @brief Helper class to write bit-packed data.
 */
class bit_writer {
public:
    void write_uint(uint64_t val, std::size_t bits) {
        if (bits == 0) {
            return;
        }
        // MSB first for bit-packed fields
        for (std::size_t i = 0; i < bits; ++i) {
            bool bit = (val >> (bits - i - 1)) & 1;
            write_bit(bit);
        }
    }

    void write_bit(bool bit) {
        const std::size_t byte_pos = m_bit_pos / BitsPerByte;
        const std::size_t bit_in_byte = m_bit_pos % BitsPerByte;

        if (byte_pos >= m_data.size()) {
            m_data.push_back(std::byte{0});
        }

        if (bit) {
            m_data[byte_pos] |= std::byte{static_cast<std::uint8_t>(1 << (BitsPerByte - 1 - bit_in_byte))};
        }
        m_bit_pos++;
    }

    void write_bytes(std::string_view bytes) {
        if (m_bit_pos % BitsPerByte != 0) {
            throw std::runtime_error("Unaligned byte write");
        }
        for (const char c : bytes) {
            m_data.push_back(static_cast<std::byte>(c));
            m_bit_pos += BitsPerByte;
        }
    }

    const std::vector<std::byte>& data() const { return m_data; }

private:
    std::vector<std::byte> m_data;
    std::size_t m_bit_pos = 0;
};

} // namespace

auto from_yaml(const descriptor& desc, const std::string& yaml_values) -> nova::expected<std::vector<std::byte>, nova::error> {
    try {
        YAML::Node values = YAML::Load(yaml_values);
        bit_writer writer;
        std::map<std::string, uint64_t> symbol_table;

        for (const auto& field_item : desc.message.fields) {
            std::size_t length_val = 0;
            if (std::holds_alternative<std::size_t>(field_item.length)) {
                length_val = std::get<std::size_t>(field_item.length);
            } else {
                const auto& ref_name = std::get<std::string>(field_item.length);
                if (symbol_table.contains(ref_name)) {
                    length_val = symbol_table.at(ref_name);
                } else {
                    return nova::unexpected(nova::error("Length reference not found: " + ref_name));
                }
            }

            std::size_t length_bits = (field_item.len_type == descriptor::length_type::byte) ? length_val * BitsPerByte : length_val;

            if (!values[field_item.name]) {
                return nova::unexpected(nova::error("Missing value for field: " + field_item.name));
            }

            if (field_item.type == descriptor::field_type::unsigned_integer) {
                uint64_t val = values[field_item.name].as<uint64_t>();
                writer.write_uint(val, length_bits);
                symbol_table[field_item.name] = val;
            } else if (field_item.type == descriptor::field_type::boolean) {
                bool val = values[field_item.name].as<bool>();
                writer.write_bit(val);
                symbol_table[field_item.name] = val ? 1 : 0;
            } else if (field_item.type == descriptor::field_type::string) {
                std::string val = values[field_item.name].as<std::string>();
                if (val.size() < length_val) {
                    val.resize(length_val, '\0');
                } else if (val.size() > length_val) {
                    val = val.substr(0, length_val);
                }
                writer.write_bytes(val);
            }
        }

        return writer.data();
    } catch (const std::exception& e) {
        return nova::unexpected(nova::error(e.what()));
    }
}

auto generate_random(const descriptor& desc) -> nova::expected<std::vector<std::byte>, nova::error> {
    try {
        bit_writer writer;
        std::map<std::string, uint64_t> symbol_table;

        // Find fields used as lengths
        std::set<std::string> length_fields;
        for (const auto& field_item : desc.message.fields) {
            if (std::holds_alternative<std::string>(field_item.length)) {
                length_fields.insert(std::get<std::string>(field_item.length));
            }
        }

        for (const auto& field_item : desc.message.fields) {
            std::size_t length_val = 0;
            if (std::holds_alternative<std::size_t>(field_item.length)) {
                length_val = std::get<std::size_t>(field_item.length);
            } else {
                const auto& ref_name = std::get<std::string>(field_item.length);
                if (symbol_table.contains(ref_name)) {
                    length_val = static_cast<std::size_t>(symbol_table.at(ref_name));
                } else {
                    return nova::unexpected(nova::error("Length reference not found: " + ref_name));
                }
            }

            const std::size_t length_bits = (field_item.len_type == descriptor::length_type::byte) ? length_val * BitsPerByte : length_val;

            if (field_item.type == descriptor::field_type::unsigned_integer) {
                uint64_t val = 0;
                if (length_fields.contains(field_item.name)) {
                    // This field is used as a length, keep it reasonable (1-16)
                    val = nova::random().number(nova::range<uint64_t>{1, 16});
                } else {
                    if (length_bits >= 64) {
                        val = nova::random().number(nova::range<uint64_t>{0, 0xFFFFFFFFFFFFFFFFULL});
                    } else {
                        val = nova::random().number(nova::range<uint64_t>{0, (1ULL << length_bits) - 1});
                    }
                }
                writer.write_uint(val, length_bits);
                symbol_table[field_item.name] = val;
            } else if (field_item.type == descriptor::field_type::boolean) {
                const bool val = nova::random().number(nova::range<int>{0, 1}) != 0;
                writer.write_bit(val);
                symbol_table[field_item.name] = val ? 1 : 0;
            } else if (field_item.type == descriptor::field_type::string) {
                const std::string val = nova::random().string<nova::ascii_distribution>(length_val);
                writer.write_bytes(val);
            }
        }

        return writer.data();
    } catch (const std::exception& e) {
        return nova::unexpected(nova::error(e.what()));
    }
}

} // namespace btx::generator
