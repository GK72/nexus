/**
 * Part of Nexus Library
 *
 * @brief   What Claude Code keeps locally: transcripts, their tokens and times,
 *          and the memory files they read.
 *
 * @author      Gábor Krisztián Girhiny
 * @created     2026-11-14
 * @updated     2026-11-14
 * @status      experimental
 * @reviewed    none
 * SPDX-License-Identifier: Unlicense
 */

#include "heimdall/claude.hpp"

#include "heimdall/style.hpp"

#include <libnova/dyn_value.hpp>

#include <fmt/format.h>

#include <algorithm>
#include <charconv>
#include <chrono>
#include <exception>
#include <fstream>
#include <functional>
#include <sstream>
#include <system_error>
#include <utility>
#include <vector>

namespace heimdall {

namespace {

/// Calls `visit` with each line of `text` that holds every one of `needles`, parsed.
void each_object(std::string_view text, std::initializer_list<std::string_view> needles,
                 const std::function<void(const nova::dyn_value&)>& visit)
{
    while (not text.empty()) {
        const auto end = text.find('\n');
        const auto line = text.substr(0, end);
        text = end == std::string_view::npos ? std::string_view() : text.substr(end + 1);
        if (not std::ranges::all_of(needles, [&](std::string_view needle) { return line.contains(needle); })) {
            continue;
        }
        const auto doc = nova::dyn_value::parse(line, nova::dyn_value::format_e::json);
        if (doc) {
            try {
                visit(*doc);
            } catch (const std::exception&) {
                // A line of another shape: not an answer or a tool call.
            }
        }
    }
}

/// The words of a shell command, split at blanks, quotes and the shell's separators.
[[nodiscard]] auto shell_words(std::string_view command) -> std::vector<std::string> {
    static constexpr std::string_view Separators = " \t\n'\";|&<>()";
    std::vector<std::string> words;
    std::string word;
    for (const auto ch : command) {
        if (Separators.contains(ch)) {
            if (not word.empty()) {
                words.push_back(std::exchange(word, {}));
            }
        } else {
            word += ch;
        }
    }
    if (not word.empty()) {
        words.push_back(std::move(word));
    }
    return words;
}

} // namespace

auto usage_t::operator+=(const usage_t& other) -> usage_t& {
    input += other.input;
    cache_creation += other.cache_creation;
    cache_read += other.cache_read;
    output += other.output;
    wall_ms += other.wall_ms;
    api_ms += other.api_ms;
    return *this;
}

auto timestamp_ms(std::string_view text) -> std::optional<std::int64_t> {
    // 2026-11-14T14:30:46.426Z; the fraction is optional.
    static constexpr std::size_t SecondsEnd = 19;
    if (text.size() < SecondsEnd or text[4] != '-' or text[7] != '-' or text[10] != 'T' or text[13] != ':'
        or text[16] != ':') {
        return std::nullopt;
    }
    const auto number = [&](std::size_t from, std::size_t count) -> std::optional<int> {
        int value = 0;
        const auto* first = text.data() + from;
        const auto [end, ec] = std::from_chars(first, first + count, value);
        return ec == std::errc() and end == first + count ? std::optional(value) : std::nullopt;
    };
    const auto year = number(0, 4);
    const auto month = number(5, 2);
    const auto day = number(8, 2);
    const auto hour = number(11, 2);
    const auto minute = number(14, 2);
    const auto second = number(17, 2);
    if (not (year and month and day and hour and minute and second)) {
        return std::nullopt;
    }
    const auto date = std::chrono::year_month_day(std::chrono::year(*year), std::chrono::month(static_cast<unsigned>(*month)),
                                                  std::chrono::day(static_cast<unsigned>(*day)));
    if (not date.ok()) {
        return std::nullopt;
    }
    auto millis = 0;
    if (text.size() > SecondsEnd and text[SecondsEnd] == '.') {
        static constexpr int Digits = 3;
        auto scale = 100;
        for (std::size_t at = SecondsEnd + 1; at < text.size() and at <= SecondsEnd + Digits; ++at) {
            if (text[at] < '0' or text[at] > '9') {
                break;
            }
            millis += (text[at] - '0') * scale;
            scale /= 10;
        }
    }
    const auto time = std::chrono::sys_days(date) + std::chrono::hours(*hour) + std::chrono::minutes(*minute)
                    + std::chrono::seconds(*second) + std::chrono::milliseconds(millis);
    return std::chrono::duration_cast<std::chrono::milliseconds>(time.time_since_epoch()).count();
}

auto duration_text(std::int64_t ms) -> std::string {
    static constexpr std::int64_t Second = 1'000;
    static constexpr std::int64_t Minute = 60;
    const auto seconds = (ms + Second / 2) / Second;
    if (seconds < Minute) {
        return fmt::format("{}s", seconds);
    }
    const auto minutes = (seconds + Minute / 2) / Minute;
    if (minutes < Minute) {
        return fmt::format("{}m", minutes);
    }
    return fmt::format("{}h{:02}m", minutes / Minute, minutes % Minute);
}

auto transcript_usage(std::string_view jsonl) -> usage_t {
    usage_t total;
    std::set<std::string> seen;
    std::map<std::string, std::int64_t> sent;         // requestRef → when sent
    std::map<std::string, std::int64_t> answered;     // requestRef → its answer's last part
    each_object(jsonl, { R"("usage")", R"("assistant")" }, [&](const nova::dyn_value& line) {
        if (line.lookup<std::string>("type", "") != "assistant") {
            return;
        }
        const auto ref = line.lookup<std::string>("requestRef", "");
        const auto when = timestamp_ms(line.lookup<std::string>("timestamp", ""));
        if (not ref.empty() and when) {
            auto& last = answered[ref];
            last = std::max(last, *when);
        }
        const auto id = line.lookup<std::string>("message.id", "");
        if (id.empty() or not seen.insert(id).second) {
            return;
        }
        total += usage_t {
            .input = line.lookup<std::int64_t>("message.usage.input_tokens", 0),
            .cache_creation = line.lookup<std::int64_t>("message.usage.cache_creation_input_tokens", 0),
            .cache_read = line.lookup<std::int64_t>("message.usage.cache_read_input_tokens", 0),
            .output = line.lookup<std::int64_t>("message.usage.output_tokens", 0),
        };
    });
    each_object(jsonl, { R"("api-request")" }, [&](const nova::dyn_value& line) {
        const auto when = timestamp_ms(line.lookup<std::string>("timestamp", ""));
        if (line.lookup<std::string>("type", "") == "api-request" and when) {
            sent[line.lookup<std::string>("id", "")] = *when;
        }
    });
    for (const auto& [ref, start] : sent) {
        const auto end = answered.find(ref);
        if (end != answered.end() and end->second > start) {
            total.api_ms += end->second - start;
        }
    }
    each_object(jsonl, { R"("turn_duration")" }, [&](const nova::dyn_value& line) {
        if (line.lookup<std::string>("type", "") == "system" and line.lookup<std::string>("subtype", "") == "turn_duration") {
            total.wall_ms += line.lookup<std::int64_t>("durationMs", 0);
        }
    });
    return total;
}

auto memory_reads(std::string_view jsonl) -> std::set<std::string> {
    std::set<std::string> files;
    const auto add = [&](const std::filesystem::path& path) {
        if (path.parent_path().filename() == "memory" and path.extension() == ".md" and path.filename() != "MEMORY.md") {
            files.insert(path.filename().string());
        }
    };
    // A word of a shell command, in a memory directory or relative to it.
    const auto add_named = [&](const std::filesystem::path& path) {
        if (path.extension() == ".md" and path.filename() != "MEMORY.md"
            and (not path.has_parent_path() or path.parent_path().filename() == "memory")) {
            files.insert(path.filename().string());
        }
    };
    each_object(jsonl, { "/memory" }, [&](const nova::dyn_value& line) {
        const auto type = line.lookup<std::string>("type", "");
        if (type == "attachment" and line.lookup<std::string>("attachment.type", "") == "file") {
            add(line.lookup<std::string>("attachment.filename", ""));
            return;
        }
        if (type != "assistant" or not line.is<nova::dyn_value::seq_t>("message.content")) {
            return;
        }
        for (const auto& part : line.at("message.content").as_seq()) {
            if (part.lookup<std::string>("type", "") != "tool_use") {
                continue;
            }
            const auto tool = part.lookup<std::string>("name", "");
            if (tool == "Read") {
                add(part.lookup<std::string>("input.file_path", ""));
            } else if (tool == "Bash") {
                const auto command = part.lookup<std::string>("input.command", "");
                if (command.contains("/memory")) {
                    for (const auto& word : shell_words(command)) {
                        add_named(word);
                    }
                }
            }
        }
    });
    return files;
}

auto tokens_text(std::int64_t tokens) -> std::string {
    static constexpr double Kilo = 1'000.0;
    static constexpr double Mega = 1'000'000.0;
    static constexpr double OneDecimalBelow = 10.0;
    const auto count = static_cast<double>(tokens);
    const auto scaled = [](double value, std::string_view unit) {
        auto text = value < OneDecimalBelow ? fmt::format("{:.1f}", value) : fmt::format("{:.0f}", value);
        if (text.ends_with(".0")) {
            text.resize(text.size() - 2);
        }
        return text + std::string(unit);
    };
    if (count >= Mega) {
        return scaled(count / Mega, "M");
    }
    if (count >= Kilo) {
        return scaled(count / Kilo, "k");
    }
    return fmt::format("{}", tokens);
}

auto claude_projects_dir() -> std::filesystem::path {
    return expand_home("~/.claude/projects");
}

auto read_text(const std::filesystem::path& path) -> std::string {
    std::ifstream file(path, std::ios::binary);
    std::ostringstream text;
    text << file.rdbuf();
    return std::move(text).str();
}

auto transcripts_in(const std::filesystem::path& dir) -> std::vector<std::filesystem::path> {
    std::vector<std::filesystem::path> ret;
    std::error_code ec;
    for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
        if (entry.path().extension() == ".jsonl" and entry.is_regular_file(ec)) {
            ret.push_back(entry.path());
        }
    }
    std::ranges::sort(ret);
    return ret;
}

auto session_usage(const std::filesystem::path& projects, const std::set<std::string>& sessions)
    -> std::map<std::string, usage_t>
{
    std::map<std::string, usage_t> ret;
    if (sessions.empty()) {
        return ret;
    }
    std::error_code ec;
    for (const auto& project : std::filesystem::directory_iterator(projects, ec)) {
        for (const auto& session : sessions) {
            const auto transcript = project.path() / (session + ".jsonl");
            if (not std::filesystem::exists(transcript, ec)) {
                continue;
            }
            auto usage = transcript_usage(read_text(transcript));
            for (const auto& agent : transcripts_in(project.path() / session / "subagents")) {
                auto agent_usage = transcript_usage(read_text(agent));
                agent_usage.wall_ms = 0;    // within the session's turns
                usage += agent_usage;
            }
            ret[session] += usage;
        }
    }
    return ret;
}

} // namespace heimdall
