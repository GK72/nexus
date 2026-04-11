#include <libbtx/descriptor.hpp>

#include <fmt/format.h>
#include <libnova/log.hpp>
#include <libnova/random.hpp>
#include <yaml-cpp/yaml.h>

#include <iomanip>
#include <set>

namespace btx {

constexpr std::size_t BitsPerByte = 8;

/**
 * @class bit_writer
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

nova::expected<descriptor, nova::error> descriptor::load(const std::string& yaml_path) {
    try {
        YAML::Node config = YAML::LoadFile(yaml_path);
        descriptor desc;
        desc.m_name = config["name"].as<std::string>();
        desc.m_version = config["version"].as<std::string>("");

        if (config["id"]) {
            if (config["id"].as<std::string>().starts_with("0x")) {
                desc.m_id = static_cast<uint32_t>(std::stoul(config["id"].as<std::string>(), nullptr, 16));
            } else {
                desc.m_id = config["id"].as<uint32_t>();
            }
        }

        auto msg_node = config["message"];
        desc.m_message.name = msg_node["name"].as<std::string>();

        for (auto field_node : msg_node["fields"]) {
            struct field f;
            f.name = field_node["name"].as<std::string>();

            std::string type_str = field_node["type"].as<std::string>("uint");
            if (type_str == "uint") {
                f.type = field_type::unsigned_integer;
            } else if (type_str == "bool") {
                f.type = field_type::boolean;
            } else if (type_str == "string") {
                f.type = field_type::string;
            } else {
                return nova::unexpected(nova::error("Unknown field type: " + type_str));
            }

            std::string len_type_str = field_node["length_type"].as<std::string>("byte");
            if (len_type_str == "bit") {
                f.len_type = length_type::bit;
            } else if (len_type_str == "byte") {
                f.len_type = length_type::byte;
            } else {
                return nova::unexpected(nova::error("Unknown length type: " + len_type_str));
            }

            auto len_node = field_node["length"];
            if (len_node.IsScalar()) {
                try {
                    f.length = len_node.as<std::size_t>();
                } catch (...) {
                    f.length = len_node.as<std::string>();
                }
            }

            desc.m_message.fields.push_back(f);
        }

        return desc;
    } catch (const std::exception& e) {
        return nova::unexpected(nova::error(e.what()));
    }
}

nova::expected<message_data, nova::error> descriptor::parse(nova::data_view data) const {
    message_data result;
    result.name = m_message.name;

    std::size_t bit_pos = 0;
    std::map<std::string, std::uint64_t> symbol_table;

    for (const auto& field_item : m_message.fields) {
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

        std::size_t length_bits = (field_item.len_type == length_type::byte) ? length_val * BitsPerByte : length_val;

        field_value fv;
        fv.name = field_item.name;

        try {
            if (field_item.type == field_type::unsigned_integer) {
                uint64_t val = data.as_number_bit_packed<uint64_t>(bit_pos, length_bits);
                fv.value = val;
                symbol_table[field_item.name] = val;
                bit_pos += length_bits;
            } else if (field_item.type == field_type::boolean) {
                bool val = data.as_number_bit_packed<uint64_t>(bit_pos, 1) != 0;
                fv.value = val;
                symbol_table[field_item.name] = val ? 1 : 0;
                bit_pos += 1;
            } else if (field_item.type == field_type::string) {
                if (bit_pos % BitsPerByte != 0) {
                    return nova::unexpected(nova::error("String field '" + field_item.name + "' must be byte-aligned"));
                }
                std::string val(data.as_string(bit_pos / BitsPerByte, length_bits / BitsPerByte));
                fv.value = val;
                bit_pos += length_bits;
            }
        } catch (const std::exception& e) {
            return nova::unexpected(nova::error("Error parsing field '" + field_item.name + "': " + e.what()));
        }

        result.fields.push_back(fv);
    }

    return result;
}

nova::expected<std::vector<std::byte>, nova::error> descriptor::generate(const std::string& yaml_values) const {
    try {
        YAML::Node values = YAML::Load(yaml_values);
        bit_writer writer;
        std::map<std::string, uint64_t> symbol_table;

        for (const auto& field_item : m_message.fields) {
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

            std::size_t length_bits = (field_item.len_type == length_type::byte) ? length_val * BitsPerByte : length_val;

            if (!values[field_item.name]) {
                return nova::unexpected(nova::error("Missing value for field: " + field_item.name));
            }

            if (field_item.type == field_type::unsigned_integer) {
                uint64_t val = values[field_item.name].as<uint64_t>();
                writer.write_uint(val, length_bits);
                symbol_table[field_item.name] = val;
            } else if (field_item.type == field_type::boolean) {
                bool val = values[field_item.name].as<bool>();
                writer.write_bit(val);
                symbol_table[field_item.name] = val ? 1 : 0;
            } else if (field_item.type == field_type::string) {
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

nova::expected<std::vector<std::byte>, nova::error> descriptor::generate_random() const {
    try {
        bit_writer writer;
        std::map<std::string, uint64_t> symbol_table;

        // Find fields used as lengths
        std::set<std::string> length_fields;
        for (const auto& field_item : m_message.fields) {
            if (std::holds_alternative<std::string>(field_item.length)) {
                length_fields.insert(std::get<std::string>(field_item.length));
            }
        }

        for (const auto& field_item : m_message.fields) {
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

            const std::size_t length_bits = (field_item.len_type == length_type::byte) ? length_val * BitsPerByte : length_val;

            if (field_item.type == field_type::unsigned_integer) {
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
            } else if (field_item.type == field_type::boolean) {
                const bool val = nova::random().number(nova::range<int>{0, 1}) != 0;
                writer.write_bit(val);
                symbol_table[field_item.name] = val ? 1 : 0;
            } else if (field_item.type == field_type::string) {
                const std::string val = nova::random().string<nova::ascii_distribution>(length_val);
                writer.write_bytes(val);
            }
        }

        return writer.data();
    } catch (const std::exception& e) {
        return nova::unexpected(nova::error(e.what()));
    }
}

nova::expected<int, nova::error> descriptor::to_btx(nova::data_view data, std::ostream& out) const {
    std::size_t bit_pos = 0;
    std::map<std::string, uint64_t> symbol_table;

    out << "// " << m_message.name << "\n";

    for (const auto& field_item : m_message.fields) {
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

        std::size_t length_bits = (field_item.len_type == length_type::byte) ? length_val * BitsPerByte : length_val;

        try {
            if (field_item.type == field_type::unsigned_integer || field_item.type == field_type::boolean) {
                uint64_t val = 0;
                if (field_item.type == field_type::unsigned_integer) {
                    val = data.as_number_bit_packed<uint64_t>(bit_pos, length_bits);
                    symbol_table[field_item.name] = val;
                } else {
                    val = data.as_number_bit_packed<uint64_t>(bit_pos, 1) != 0;
                    symbol_table[field_item.name] = val;
                    length_bits = 1;
                }

                std::ostringstream btx_ss;
                if (length_bits % BitsPerByte == 0 && bit_pos % BitsPerByte == 0) {
                    for (std::size_t i = 0; i < length_bits / BitsPerByte; ++i) {
                        btx_ss << fmt::format("\\x{:02X}", static_cast<uint32_t>(data.span()[bit_pos / BitsPerByte + i]));
                    }
                } else {
                    btx_ss << "\\b";
                    for (std::size_t i = 0; i < length_bits; ++i) {
                        bool bit = data.as_number_bit_packed<uint64_t>(bit_pos + i, 1) != 0;
                        btx_ss << (bit ? '1' : '0');
                    }
                }
                out << std::left << std::setw(40) << btx_ss.str() << " // " << field_item.name << ": " << val << " (0x" << std::hex << val << std::dec << ")\n";
                bit_pos += length_bits;
            } else if (field_item.type == field_type::string) {
                if (bit_pos % BitsPerByte != 0) {
                    return nova::unexpected(nova::error("String field '" + field_item.name + "' must be byte-aligned"));
                }
                std::string val(data.as_string(bit_pos / BitsPerByte, length_bits / BitsPerByte));
                for (std::size_t i = 0; i < length_bits / BitsPerByte; ++i) {
                    out << fmt::format("\\x{:02X}", static_cast<uint32_t>(data.span()[bit_pos / BitsPerByte + i]));
                    if ((i + 1) % BitsPerByte == 0 || (i + 1) == length_bits / BitsPerByte) {
                        if ((i + 1) == length_bits / BitsPerByte) {
                            std::size_t current_line_bytes = (i % BitsPerByte) + 1;
                            if (current_line_bytes <= 8) {
                                out << std::string((10 - current_line_bytes) * 4, ' ');
                            }
                            out << " // " << field_item.name << ": \"" << val << "\"";
                        }
                        out << "\n";
                    }
                }
                bit_pos += length_bits;
            }
        } catch (const std::exception& e) {
            return nova::unexpected(nova::error("Error processing field '" + field_item.name + "': " + e.what()));
        }
    }

    return 0;
}

} // namespace btx
