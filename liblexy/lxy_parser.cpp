#include <liblexy/descriptor.hpp>

#include <libnova/expected.hpp>
#include <libnova/io.hpp>

#include <cctype>
#include <string_view>
#include <vector>

namespace lexy {

namespace {

enum class token_type {
    kw_message,
    kw_int,
    kw_str,
    kw_bool,
    identifier,
    number,
    ref_identifier,
    lbrace,
    rbrace,
    colon,
    semicolon,
    eof,
    unknown
};

struct token {
    token_type type;
    std::string_view value;
};

class lexer {
public:
    explicit lexer(std::string_view content)
        : m_content(content)
    {}

    auto next() -> token {
        skip_whitespace();

        if (m_cursor >= m_content.size()) {
            return { token_type::eof, "" };
        }

        char c = m_content[m_cursor];
        if (std::isalpha(c) || c == '_') {
            return consume_identifier();
        }

        if (std::isdigit(c)) {
            return consume_number();
        }

        if (c == '@') {
            return consume_reference();
        }

        m_cursor++;
        switch (c) {
            case '{': return { token_type::lbrace, "{" };
            case '}': return { token_type::rbrace, "}" };
            case ':': return { token_type::colon, ":" };
            case ';': return { token_type::semicolon, ";" };
            default:  return { token_type::unknown, m_content.substr(m_cursor - 1, 1) };
        }
    }

private:
    std::string_view m_content;
    std::size_t m_cursor = 0;

    void skip_whitespace() {
        while (m_cursor < m_content.size() && std::isspace(m_content[m_cursor])) {
            m_cursor++;
        }
    }

    auto consume_identifier() -> token {
        std::size_t start = m_cursor;

        while (m_cursor < m_content.size() && (std::isalnum(m_content[m_cursor]) || m_content[m_cursor] == '_')) {
            m_cursor++;
        }

        std::string_view value = m_content.substr(start, m_cursor - start);

        if (value == "message") { return { token_type::kw_message, value }; }
        if (value == "int")     { return { token_type::kw_int, value }; }
        if (value == "str")     { return { token_type::kw_str, value }; }
        if (value == "bool")    { return { token_type::kw_bool, value }; }

        return { token_type::identifier, value };
    }

    auto consume_number() -> token {
        std::size_t start = m_cursor;

        while (m_cursor < m_content.size() && std::isdigit(m_content[m_cursor])) {
            m_cursor++;
        }

        return { token_type::number, m_content.substr(start, m_cursor - start) };
    }

    /**
     * @brief   Skip one character indicating reference.
     */
    auto consume_reference() -> token {
        m_cursor++; // skip '@'
        std::size_t start = m_cursor;

        while (m_cursor < m_content.size() && (std::isalnum(m_content[m_cursor]) || m_content[m_cursor] == '_')) {
            m_cursor++;
        }

        return { token_type::ref_identifier, m_content.substr(start, m_cursor - start) };
    }

};

class parser {
public:
    explicit parser(std::string_view content)
        : m_lexer(content)
    {
        m_current = m_lexer.next();
    }

    auto parse() -> nova::expected<descriptor, nova::error> {
        descriptor desc;
        desc.name = "Parsed LXY Descriptor";
        desc.version = "1.0.0";
        desc.message.id = 0;

        if (m_current.type != token_type::kw_message) {
            return nova::unexpected(nova::error("Expected 'message' keyword"));
        }
        consume();

        if (m_current.type != token_type::identifier) {
            return nova::unexpected(nova::error("Expected message name"));
        }
        desc.message.name = std::string(m_current.value);
        consume();

        if (m_current.type != token_type::lbrace) {
            return nova::unexpected(nova::error("Expected '{'"));
        }
        consume();

        while (m_current.type != token_type::rbrace && m_current.type != token_type::eof) {
            auto field = parse_field();
            if (not field) {
                return nova::unexpected(field.error());
            }

            desc.message.fields.push_back(*field);
        }

        if (m_current.type != token_type::rbrace) {
            return nova::unexpected(nova::error("Expected '}'"));
        }
        consume();

        if (desc.message.fields.empty()) {
            return nova::unexpected(nova::error("No fields found in message"));
        }

        return desc;
    }

private:
    lexer m_lexer;
    token m_current;

    auto parse_field() -> nova::expected<descriptor::field, nova::error> {
        descriptor::field field;

        if (m_current.type == token_type::kw_int) {
            field.type = descriptor::field_type::unsigned_integer;
            field.len_type = descriptor::length_type::bit;
        } else if (m_current.type == token_type::kw_str) {
            field.type = descriptor::field_type::string;
            field.len_type = descriptor::length_type::byte;
        } else if (m_current.type == token_type::kw_bool) {
            field.type = descriptor::field_type::boolean;
            field.len_type = descriptor::length_type::bit;
        } else {
            return nova::unexpected(nova::error("Expected field type (int, str, bool)"));
        }
        consume();

        if (m_current.type != token_type::identifier) {
            return nova::unexpected(nova::error("Expected field name"));
        }
        field.name = std::string(m_current.value);
        consume();

        if (m_current.type != token_type::colon) {
            return nova::unexpected(nova::error("Expected ':'"));
        }
        consume();

        if (m_current.type == token_type::number) {
            try {
                field.length = std::stoul(std::string(m_current.value));
            } catch (...) {
                return nova::unexpected(nova::error("Invalid length value: " + std::string(m_current.value)));
            }
        } else if (m_current.type == token_type::ref_identifier) {
            field.length = std::string(m_current.value);
        } else {
            return nova::unexpected(nova::error("Expected length (number or @reference)"));
        }
        consume();

        if (m_current.type != token_type::semicolon) {
            return nova::unexpected(nova::error("Expected ';'"));
        }
        consume();

        return field;
    }

    void consume() {
        m_current = m_lexer.next();
    }

};

} // namespace

[[nodiscard]] auto load_descriptor_lxy(std::string_view content)
        -> nova::expected<descriptor, nova::error>
{
    auto p = parser{ content };
    return p.parse();
}

[[nodiscard]] auto load_descriptor_lxy(const std::filesystem::path& path)
        -> nova::expected<descriptor, nova::error>
{
    auto content = nova::read_file(path);

    if (not content) {
        return nova::unexpected{ content.error() };
    }

    return load_descriptor_lxy(*content);
}

} // namespace lexy
