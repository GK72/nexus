#include <liblexy/descriptor.hpp>
#include <libnova/io.hpp>
#include <string_view>
#include <fstream>
#include <sstream>
#include <vector>
#include <cctype>

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
    explicit lexer(std::string_view content) : content_(content) {}

    auto next() -> token {
        skip_whitespace();
        if (cursor_ >= content_.size()) {
            return {token_type::eof, ""};
        }

        char c = content_[cursor_];
        if (std::isalpha(c) || c == '_') {
            return consume_identifier();
        }
        if (std::isdigit(c)) {
            return consume_number();
        }
        if (c == '@') {
            return consume_reference();
        }

        cursor_++;
        switch (c) {
            case '{': return {token_type::lbrace, "{"};
            case '}': return {token_type::rbrace, "}"};
            case ':': return {token_type::colon, ":"};
            case ';': return {token_type::semicolon, ";"};
            default: return {token_type::unknown, content_.substr(cursor_ - 1, 1)};
        }
    }

private:
    void skip_whitespace() {
        while (cursor_ < content_.size() && std::isspace(content_[cursor_])) {
            cursor_++;
        }
    }

    auto consume_identifier() -> token {
        size_t start = cursor_;
        while (cursor_ < content_.size() && (std::isalnum(content_[cursor_]) || content_[cursor_] == '_')) {
            cursor_++;
        }
        std::string_view val = content_.substr(start, cursor_ - start);
        if (val == "message") return {token_type::kw_message, val};
        if (val == "int") return {token_type::kw_int, val};
        if (val == "str") return {token_type::kw_str, val};
        if (val == "bool") return {token_type::kw_bool, val};
        return {token_type::identifier, val};
    }

    auto consume_number() -> token {
        size_t start = cursor_;
        while (cursor_ < content_.size() && std::isdigit(content_[cursor_])) {
            cursor_++;
        }
        return {token_type::number, content_.substr(start, cursor_ - start)};
    }

    auto consume_reference() -> token {
        cursor_++; // skip '@'
        size_t start = cursor_;
        while (cursor_ < content_.size() && (std::isalnum(content_[cursor_]) || content_[cursor_] == '_')) {
            cursor_++;
        }
        return {token_type::ref_identifier, content_.substr(start, cursor_ - start)};
    }

    std::string_view content_;
    size_t cursor_ = 0;
};

class parser {
public:
    explicit parser(std::string_view content) : lexer_(content) {
        current_ = lexer_.next();
    }

    auto parse() -> nova::expected<descriptor, nova::error> {
        descriptor desc;
        desc.name = "Parsed LXY Descriptor";
        desc.version = "1.0.0";
        desc.message.id = 0;

        if (current_.type != token_type::kw_message) {
            return nova::unexpected(nova::error("Expected 'message' keyword"));
        }
        consume();

        if (current_.type != token_type::identifier) {
            return nova::unexpected(nova::error("Expected message name"));
        }
        desc.message.name = std::string(current_.value);
        consume();

        if (current_.type != token_type::lbrace) {
            return nova::unexpected(nova::error("Expected '{'"));
        }
        consume();

        while (current_.type != token_type::rbrace && current_.type != token_type::eof) {
            auto field_res = parse_field();
            if (!field_res) return nova::unexpected(field_res.error());
            desc.message.fields.push_back(*field_res);
        }

        if (current_.type != token_type::rbrace) {
            return nova::unexpected(nova::error("Expected '}'"));
        }
        consume();

        if (desc.message.fields.empty()) {
            return nova::unexpected(nova::error("No fields found in message"));
        }

        return desc;
    }

private:
    auto parse_field() -> nova::expected<descriptor::field, nova::error> {
        descriptor::field f;

        if (current_.type == token_type::kw_int) {
            f.type = descriptor::field_type::unsigned_integer;
            f.len_type = descriptor::length_type::bit;
        } else if (current_.type == token_type::kw_str) {
            f.type = descriptor::field_type::string;
            f.len_type = descriptor::length_type::byte;
        } else if (current_.type == token_type::kw_bool) {
            f.type = descriptor::field_type::boolean;
            f.len_type = descriptor::length_type::bit;
        } else {
            return nova::unexpected(nova::error("Expected field type (int, str, bool)"));
        }
        consume();

        if (current_.type != token_type::identifier) {
            return nova::unexpected(nova::error("Expected field name"));
        }
        f.name = std::string(current_.value);
        consume();

        if (current_.type != token_type::colon) {
            return nova::unexpected(nova::error("Expected ':'"));
        }
        consume();

        if (current_.type == token_type::number) {
            try {
                f.length = std::stoul(std::string(current_.value));
            } catch (...) {
                return nova::unexpected(nova::error("Invalid length value: " + std::string(current_.value)));
            }
        } else if (current_.type == token_type::ref_identifier) {
            f.length = std::string(current_.value);
        } else {
            return nova::unexpected(nova::error("Expected length (number or @reference)"));
        }
        consume();

        if (current_.type != token_type::semicolon) {
            return nova::unexpected(nova::error("Expected ';'"));
        }
        consume();

        return f;
    }

    void consume() {
        current_ = lexer_.next();
    }

    lexer lexer_;
    token current_;
};

} // namespace

[[nodiscard]] auto load_descriptor_lxy(std::string_view lxy_content)
        -> nova::expected<descriptor, nova::error>
{
    parser p(lxy_content);
    return p.parse();
}

[[nodiscard]] auto load_descriptor_lxy(const std::filesystem::path& lxy_path)
        -> nova::expected<descriptor, nova::error>
{
    std::ifstream ifs(lxy_path);
    if (!ifs) {
        return nova::unexpected(nova::error("Could not open file: " + lxy_path.string()));
    }
    std::stringstream ss;
    ss << ifs.rdbuf();
    return load_descriptor_lxy(ss.str());
}

} // namespace lexy
