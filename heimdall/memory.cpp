/**
 * Part of Nexus Library
 *
 * @brief   Claude's memory: its files, their sizes, how often they are read.
 *
 * @author      Gábor Krisztián Girhiny
 * @created     2026-11-14
 * @updated     2026-11-14
 * @status      experimental
 * @reviewed    none
 * SPDX-License-Identifier: Unlicense
 */

#include "heimdall/memory.hpp"

#include "heimdall/claude.hpp"

#include <fmt/format.h>
#include <fmt/ranges.h>

#include <algorithm>
#include <charconv>
#include <chrono>
#include <cstdlib>
#include <map>
#include <ranges>
#include <system_error>
#include <utility>

namespace heimdall {

namespace {

constexpr std::size_t MinReads = 5;
constexpr std::size_t MinPercent = 15;
constexpr std::size_t MaxFileWidth = 32;
constexpr std::size_t TypeWidth = 9;
constexpr std::size_t SizeWidth = 7;
constexpr std::size_t TokensWidth = 6;
constexpr std::size_t ReadsWidth = 7;
constexpr std::size_t DateWidth = 10;
constexpr std::string_view Index = "MEMORY.md";

[[nodiscard]] auto trim(std::string_view text) -> std::string_view {
    while (not text.empty() and (text.front() == ' ' or text.front() == '\t')) {
        text.remove_prefix(1);
    }
    while (not text.empty() and (text.back() == ' ' or text.back() == '\r')) {
        text.remove_suffix(1);
    }
    if (text.size() >= 2 and (text.front() == '"' or text.front() == '\'') and text.back() == text.front()) {
        text = text.substr(1, text.size() - 2);
    }
    return text;
}

constexpr std::size_t Percent = 100;
constexpr std::uintmax_t BytesPerKilo = 1'000;
constexpr std::uintmax_t BytesPerToken = 4;

[[nodiscard]] auto size_text(std::uintmax_t bytes) -> std::string {
    return bytes < BytesPerKilo ? fmt::format("{} B", bytes)
                                : fmt::format("{:.1f} KB", static_cast<double>(bytes) / static_cast<double>(BytesPerKilo));
}

[[nodiscard]] auto tokens_of(std::uintmax_t bytes) -> std::string {
    return tokens_text(static_cast<std::int64_t>(approx_tokens(bytes)));
}

[[nodiscard]] auto date_of(const std::filesystem::path& path) -> std::string {
    std::error_code ec;
    const auto written = std::filesystem::last_write_time(path, ec);
    if (ec) {
        return {};
    }
    const auto when = std::chrono::clock_cast<std::chrono::system_clock>(written);
    const std::chrono::year_month_day date { std::chrono::floor<std::chrono::days>(when) };
    return fmt::format("{:04}-{:02}-{:02}", static_cast<int>(date.year()), static_cast<unsigned>(date.month()),
                       static_cast<unsigned>(date.day()));
}

[[nodiscard]] auto promotions(const std::vector<memory_t>& memories) -> std::size_t {
    std::size_t count = 0;
    for (const auto& memory : memories) {
        count += static_cast<std::size_t>(std::ranges::count_if(memory.files, [&](const memory_file_t& file) {
            return not promotion_hint(file, memory.sessions).empty();
        }));
    }
    return count;
}

} // namespace

auto parse_scan_cache(std::string_view text) -> scan_cache_t {
    scan_cache_t cache;
    for (const auto line : std::views::split(text, '\n')) {
        std::vector<std::string_view> fields;
        for (const auto field : std::views::split(std::string_view(line.begin(), line.end()), '\t')) {
            fields.emplace_back(field.begin(), field.end());
        }
        if (fields.size() != 4) {
            continue;
        }
        scan_t scan;
        const auto size = std::from_chars(fields[1].data(), fields[1].data() + fields[1].size(), scan.size);
        const auto written = std::from_chars(fields[2].data(), fields[2].data() + fields[2].size(), scan.written);
        if (size.ec != std::errc{} or written.ec != std::errc{}) {
            continue;
        }
        for (const auto file : std::views::split(fields[3], ',')) {
            if (not file.empty()) {
                scan.files.emplace(file.begin(), file.end());
            }
        }
        cache.emplace(std::string(fields[0]), std::move(scan));
    }
    return cache;
}

auto format_scan_cache(const scan_cache_t& cache) -> std::string {
    std::string text;
    for (const auto& [path, scan] : cache) {
        text += fmt::format("{}\t{}\t{}\t{}\n", path, scan.size, scan.written, fmt::join(scan.files, ","));
    }
    return text;
}

auto scan_cache_path() -> std::filesystem::path {
    const char* xdg = std::getenv("XDG_CACHE_HOME");  // NOLINT(concurrency-mt-unsafe): read-only
    const auto base = xdg != nullptr and *xdg != '\0' ? std::filesystem::path(xdg)
                                                       : std::filesystem::path(expand_home("~/.cache"));
    return base / "heimdall" / "memory-reads.tsv";
}

auto memory_t::bytes() const -> std::uintmax_t {
    std::uintmax_t total = index_bytes;
    for (const auto& file : files) {
        total += file.bytes;
    }
    return total;
}

auto parse_front_matter(std::string_view text) -> front_matter_t {
    front_matter_t about;
    if (not text.starts_with("---")) {
        return about;
    }
    text.remove_prefix(text.find('\n') == std::string_view::npos ? text.size() : text.find('\n') + 1);
    while (not text.empty()) {
        const auto end = text.find('\n');
        const auto line = text.substr(0, end);
        text = end == std::string_view::npos ? std::string_view() : text.substr(end + 1);
        if (trim(line) == "---") {
            break;
        }
        const auto colon = line.find(':');
        if (colon == std::string_view::npos) {
            continue;
        }
        const auto key = trim(line.substr(0, colon));
        const auto value = std::string(trim(line.substr(colon + 1)));
        if (key == "name") {
            about.name = value;
        } else if (key == "description") {
            about.description = value;
        } else if (key == "type") {
            about.type = value;
        }
    }
    return about;
}

auto approx_tokens(std::uintmax_t bytes) -> std::uintmax_t {
    return (bytes + BytesPerToken - 1) / BytesPerToken;
}

auto promotion_hint(const memory_file_t& file, std::size_t sessions) -> std::string {
    if (file.reads < MinReads or file.reads * Percent < sessions * MinPercent) {
        return {};
    }
    if (file.about.type == "reference") {
        return "→ doc or skill?";
    }
    if (file.about.type == "project") {
        return "→ project doc?";
    }
    if (file.about.type == "feedback") {
        return "→ CLAUDE.md?";
    }
    return {};
}

auto memory_dirs(const std::vector<std::string>& configured, const std::filesystem::path& projects)
    -> std::vector<std::filesystem::path>
{
    std::vector<std::filesystem::path> ret;
    for (const auto& dir : configured) {
        ret.emplace_back(expand_home(dir));
    }
    if (not configured.empty()) {
        return ret;
    }
    std::error_code ec;
    for (const auto& project : std::filesystem::directory_iterator(projects, ec)) {
        if (std::filesystem::is_directory(project.path() / "memory", ec)) {
            ret.push_back(project.path() / "memory");
        }
    }
    std::ranges::sort(ret);
    return ret;
}

auto load_memory(const std::filesystem::path& dir, const scan_cache_t& cached, scan_cache_t& scanned) -> memory_t {
    memory_t memory { .dir = dir };
    std::error_code ec;
    for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
        const auto& path = entry.path();
        if (path.extension() != ".md" or not entry.is_regular_file(ec)) {
            continue;
        }
        const auto bytes = entry.file_size(ec);
        if (path.filename() == Index) {
            memory.index_bytes = bytes;
            continue;
        }
        memory.files.push_back({
            .file = path.filename().string(),
            .about = parse_front_matter(read_text(path)),
            .bytes = bytes,
            .changed = date_of(path),
        });
    }

    std::map<std::string, std::size_t> reads;
    const auto transcripts = transcripts_in(dir.parent_path());
    memory.sessions = transcripts.size();
    for (const auto& transcript : transcripts) {
        const auto key = transcript.string();
        scan_t scan { .size = std::filesystem::file_size(transcript, ec),
                      .written = std::filesystem::last_write_time(transcript, ec).time_since_epoch().count() };
        const auto known = cached.find(key);
        if (known != cached.end() and known->second.size == scan.size and known->second.written == scan.written) {
            scan.files = known->second.files;
        } else {
            scan.files = memory_reads(read_text(transcript));
        }
        for (const auto& file : scan.files) {
            ++reads[file];
        }
        scanned.insert_or_assign(key, std::move(scan));
    }
    for (auto& file : memory.files) {
        const auto found = reads.find(file.file);
        file.reads = found != reads.end() ? found->second : 0;
    }
    std::ranges::sort(memory.files, [](const memory_file_t& lhs, const memory_file_t& rhs) {
        return std::pair(rhs.reads, lhs.file) < std::pair(lhs.reads, rhs.file);
    });
    return memory;
}

auto render_memory(const style_t& style, const std::vector<memory_t>& memories, std::size_t width) -> std::string {
    const auto separator = style(" · ", "faint");
    std::size_t files = 0;
    std::uintmax_t bytes = 0;
    std::uintmax_t index = 0;
    for (const auto& memory : memories) {
        files += memory.files.size();
        bytes += memory.bytes();
        index += memory.index_bytes;
    }
    const auto totals = style(fmt::format("{} files", files), "text") + separator + style(size_text(bytes), "dim")
                      + separator + style(fmt::format("~{} tok", tokens_of(bytes)), "dim");
    std::vector<std::string> out { "", spread(style("Memory", "title", true), totals, width) };
    out.push_back(spread("", style(fmt::format("index {} (~{} tok) loaded every session", size_text(index),
                                               tokens_of(index)), "dim"), width));
    if (memories.empty()) {
        out.emplace_back();
        out.push_back(style("no memory directory", "dim"));
    }

    std::size_t name_width = 4;
    for (const auto& memory : memories) {
        for (const auto& file : memory.files) {
            name_width = std::max(name_width, file.file.size());
        }
    }
    name_width = std::min(name_width, MaxFileWidth);

    for (const auto& memory : memories) {
        out.emplace_back();
        out.push_back(rule(style, home_path(memory.dir.string()), width,
                           style(fmt::format("{} file{} · {} session{}", memory.files.size(),
                                             memory.files.size() == 1 ? "" : "s", memory.sessions,
                                             memory.sessions == 1 ? "" : "s"), "dim")));
        out.push_back(style(fmt::format("{:<{}}  {:<{}} {:>{}} {:>{}} {:>{}}  {:<{}}  {}", "FILE", name_width, "TYPE",
                                        TypeWidth, "SIZE", SizeWidth, "~TOK", TokensWidth, "READ IN", ReadsWidth,
                                        "CHANGED", DateWidth, "HINT"),
                            "faint"));
        for (const auto& file : memory.files) {
            const auto hint = promotion_hint(file, memory.sessions);
            out.push_back(pad(style(ellipsize(file.file, name_width), "text"), name_width) + "  "
                          + pad(style(file.about.type.empty() ? "-" : file.about.type, "dim"), TypeWidth) + " "
                          + style(fmt::format("{:>{}}", size_text(file.bytes), SizeWidth), "text") + " "
                          + style(fmt::format("{:>{}}", tokens_of(file.bytes), TokensWidth), "dim")
                          + " "
                          + style(fmt::format("{:>{}}", fmt::format("{}/{}", file.reads, memory.sessions), ReadsWidth),
                                  file.reads > 0 ? "text" : "faint")
                          + "  " + style(fmt::format("{:<{}}", file.changed, DateWidth), "dim")
                          + (hint.empty() ? "" : "  " + style(hint, "yellow")));
        }
    }

    std::string text;
    for (const auto& line : out) {
        text += (line.empty() ? "" : "  " + line) + "\n";
    }
    return text;
}

auto memory_summary(const style_t& style, const std::vector<memory_t>& memories) -> std::string {
    const auto separator = style(" · ", "faint");
    std::size_t files = 0;
    std::uintmax_t bytes = 0;
    for (const auto& memory : memories) {
        files += memory.files.size();
        bytes += memory.bytes();
    }
    auto line = style(fmt::format("{} files", files), "text") + separator + style(size_text(bytes), "dim") + separator
              + style(fmt::format("~{} tok", tokens_of(bytes)), "dim");
    if (const auto promote = promotions(memories); promote > 0) {
        line += separator + style(fmt::format("{} to promote", promote), "yellow") + style("  heimdall memory", "faint");
    }
    return line;
}

} // namespace heimdall
