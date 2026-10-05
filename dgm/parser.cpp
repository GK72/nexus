/**
 * Part of Nexus DGM (Diagram as Code).
 *
 * Line-based parser: text to `spec`. Only syntax is checked here; references
 * between boxes are resolved by `layout`.
 *
 * @author      Gábor Krisztián Girhiny
 * @coauthor    Claude Opus 5.5
 * @date        2026-10-05
 */

#include <dgm/diagram.hpp>

#include <fmt/format.h>

#include <algorithm>
#include <charconv>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace dgm {

namespace {

/**
 * @brief   A word or a quoted string from one line of source.
 */
struct token {
    std::string text {};
    bool quoted { false };
};

/**
 * @brief   Convenience alias for the parser's error-or-value results.
 */
template <typename T>
using result = nova::expected<T, nova::error>;

/**
 * @brief   Build an error carrying the line number.
 */
template <typename ...Args>
[[nodiscard]] auto fail(int line, fmt::format_string<Args...> fmt, Args&& ...args) -> nova::unexpected<nova::error> {
    return nova::unexpected(nova::error(
        fmt::format("line {}: {}", line, fmt::format(fmt, std::forward<Args>(args)...))
    ));
}

/**
 * @brief   Split one line into tokens, dropping a trailing `//` comment.
 *
 * @param   text    Line without the newline.
 * @param   line    Line number for error messages.
 *
 * @return  Tokens in order, or an error for an unterminated string.
 */
[[nodiscard]] auto tokenize(std::string_view text, int line) -> result<std::vector<token>> {
    std::vector<token> tokens;
    std::size_t pos = 0;

    while (pos < text.size()) {
        const char ch = text[pos];

        if (ch == ' ' or ch == '\t' or ch == '\r') {
            ++pos;
            continue;
        }

        if (text.substr(pos).starts_with("//")) {
            break;
        }

        if (ch == '"') {
            token tok { .text = {}, .quoted = true };
            ++pos;
            bool closed = false;
            while (pos < text.size()) {
                const char c = text[pos];
                if (c == '\\' and pos + 1 < text.size() and (text[pos + 1] == '"' or text[pos + 1] == '\\')) {
                    tok.text.push_back(text[pos + 1]);
                    pos += 2;
                    continue;
                }
                if (c == '"') {
                    closed = true;
                    ++pos;
                    break;
                }
                tok.text.push_back(c);
                ++pos;
            }
            if (not closed) {
                return fail(line, "unterminated string");
            }
            tokens.push_back(std::move(tok));
            continue;
        }

        const auto end = text.find_first_of(" \t\r\"", pos);
        const auto word = text.substr(pos, end == std::string_view::npos ? std::string_view::npos : end - pos);
        tokens.push_back(token { .text = std::string{ word }, .quoted = false });
        pos += word.size();
    }

    return tokens;
}

/**
 * @brief   Parse a whole string as a decimal integer.
 */
[[nodiscard]] auto to_int(std::string_view text) -> std::optional<int> {
    int value = 0;
    const auto* const first = text.data();
    const auto* const last = text.data() + text.size();
    const auto [ptr, ec] = std::from_chars(first, last, value);
    if (text.empty() or ec != std::errc{} or ptr != last) {
        return std::nullopt;
    }
    return value;
}

/**
 * @brief   Split `text` on the first `sep` and parse both halves as integers.
 */
[[nodiscard]] auto to_int_pair(std::string_view text, char sep) -> std::optional<std::pair<int, int>> {
    const auto at = text.find(sep);
    if (at == std::string_view::npos) {
        return std::nullopt;
    }
    const auto first = to_int(text.substr(0, at));
    const auto second = to_int(text.substr(at + 1));
    if (not first or not second) {
        return std::nullopt;
    }
    return std::pair { *first, *second };
}

/**
 * @brief   Whether `tok` is a valid unquoted identifier.
 */
[[nodiscard]] auto is_identifier(const token& tok) -> bool {
    return not tok.quoted
        and not tok.text.empty()
        and std::ranges::all_of(tok.text, [](char c) {
            return (c >= 'a' and c <= 'z') or (c >= 'A' and c <= 'Z') or (c >= '0' and c <= '9') or c == '_' or c == '-';
        });
}

/**
 * @brief   Map a placement keyword to its direction.
 */
[[nodiscard]] auto to_direction(const token& tok) -> std::optional<direction> {
    if (tok.quoted) {
        return std::nullopt;
    }
    if (tok.text == "right-of") { return direction::right_of; }
    if (tok.text == "left-of")  { return direction::left_of; }
    if (tok.text == "above")    { return direction::above; }
    if (tok.text == "below")    { return direction::below; }
    return std::nullopt;
}

/**
 * @brief   Parse `box <id> ["label"] [placement] [size WxH]`.
 */
[[nodiscard]] auto parse_box(const std::vector<token>& tokens, int line) -> result<box_spec> {
    if (tokens.size() < 2 or not is_identifier(tokens[1])) {
        return fail(line, "invalid box id '{}'", tokens.size() < 2 ? "" : tokens[1].text);
    }

    box_spec box { .id = tokens[1].text, .label = tokens[1].text, .place = flow{}, .size = {}, .line = line };
    std::size_t i = 2;

    if (i < tokens.size() and tokens[i].quoted) {
        box.label = tokens[i].text;
        ++i;
    }

    bool has_place = false;
    bool has_size = false;

    while (i < tokens.size()) {
        const auto& keyword = tokens[i];
        const auto value = [&]() -> std::string {
            return i + 1 < tokens.size() ? tokens[i + 1].text : std::string{};
        };

        if (not keyword.quoted and keyword.text == "size") {
            if (has_size) {
                return fail(line, "size given more than once");
            }
            const auto dims = to_int_pair(value(), 'x');
            if (not dims or dims->first <= 0 or dims->second <= 0) {
                return fail(line, "invalid size '{}', expected WxH", value());
            }
            box.size = extent { .width = dims->first, .height = dims->second };
            has_size = true;
            i += 2;
            continue;
        }

        if (not keyword.quoted and keyword.text == "at") {
            if (has_place) {
                return fail(line, "placement given more than once");
            }
            const auto pos = to_int_pair(value(), ',');
            if (not pos) {
                return fail(line, "invalid position '{}', expected X,Y", value());
            }
            box.place = absolute { .x = pos->first, .y = pos->second };
            has_place = true;
            i += 2;
            continue;
        }

        if (const auto side = to_direction(keyword)) {
            if (has_place) {
                return fail(line, "placement given more than once");
            }
            if (i + 1 >= tokens.size() or not is_identifier(tokens[i + 1])) {
                return fail(line, "invalid box id '{}'", value());
            }
            relative rel { .side = *side, .anchor = tokens[i + 1].text, .gap = DefaultGap };
            i += 2;

            if (i < tokens.size() and not tokens[i].quoted and tokens[i].text == "gap") {
                const auto text = i + 1 < tokens.size() ? tokens[i + 1].text : std::string{};
                const auto gap = to_int(text);
                if (not gap or *gap < 0) {
                    return fail(line, "invalid gap '{}'", text);
                }
                rel.gap = *gap;
                i += 2;
            }

            box.place = std::move(rel);
            has_place = true;
            continue;
        }

        return fail(line, "unexpected token '{}'", keyword.text);
    }

    return box;
}

/**
 * @brief   Parse `arrow <from> -> <to> ["label"]`.
 */
[[nodiscard]] auto parse_arrow(const std::vector<token>& tokens, int line) -> result<arrow_spec> {
    if (tokens.size() < 2 or not is_identifier(tokens[1])) {
        return fail(line, "invalid box id '{}'", tokens.size() < 2 ? "" : tokens[1].text);
    }
    if (tokens.size() < 3 or tokens[2].quoted or tokens[2].text != "->") {
        return fail(line, "expected '->' in arrow");
    }
    if (tokens.size() < 4 or not is_identifier(tokens[3])) {
        return fail(line, "invalid box id '{}'", tokens.size() < 4 ? "" : tokens[3].text);
    }

    arrow_spec arr { .from = tokens[1].text, .to = tokens[3].text, .label = {}, .line = line };
    std::size_t i = 4;

    if (i < tokens.size() and tokens[i].quoted) {
        arr.label = tokens[i].text;
        ++i;
    }
    if (i < tokens.size()) {
        return fail(line, "unexpected token '{}'", tokens[i].text);
    }

    return arr;
}

} // namespace

auto parse(std::string_view source) -> nova::expected<spec, nova::error> {
    spec out;
    int line = 0;
    std::size_t start = 0;

    while (start <= source.size()) {
        ++line;
        const auto end = source.find('\n', start);
        const auto text = source.substr(start, end == std::string_view::npos ? std::string_view::npos : end - start);
        start = end == std::string_view::npos ? source.size() + 1 : end + 1;

        const auto tokens = tokenize(text, line);
        if (not tokens) {
            return nova::unexpected(tokens.error());
        }
        if (tokens->empty()) {
            continue;
        }

        const auto& head = tokens->front();
        if (not head.quoted and head.text == "box") {
            auto parsed = parse_box(*tokens, line);
            if (not parsed) {
                return nova::unexpected(parsed.error());
            }
            out.boxes.push_back(std::move(*parsed));
        } else if (not head.quoted and head.text == "arrow") {
            auto parsed = parse_arrow(*tokens, line);
            if (not parsed) {
                return nova::unexpected(parsed.error());
            }
            out.arrows.push_back(std::move(*parsed));
        } else {
            return fail(line, "unknown statement '{}'", head.text);
        }
    }

    return out;
}

} // namespace dgm
