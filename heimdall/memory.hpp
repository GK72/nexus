/**
 * Part of Nexus Library
 *
 * @brief   Claude's memory: its files, their sizes, and how often sessions
 *          read them, with a hint when one is read so often that it belongs
 *          in documentation, a skill or `CLAUDE.md`.
 *
 * A memory directory is `<projects>/<slug>/memory`: an index `MEMORY.md`,
 * loaded in every session, and one file per fact, read when needed. The
 * sessions are the transcripts next to it.
 *
 * @author      Gábor Krisztián Girhiny
 * @created     2026-11-14
 * @updated     2026-11-14
 * @status      experimental
 * @reviewed    none
 * SPDX-License-Identifier: Unlicense
 */

#pragma once

#include "heimdall/style.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace heimdall {

/// What a memory file's front matter says.
struct front_matter_t {
    std::string name;
    std::string description;
    std::string type;                       ///< user, feedback, project or reference
};

struct memory_file_t {
    std::string file;                       ///< `norn-service.md`
    front_matter_t about;
    std::uintmax_t bytes = 0;
    std::size_t reads = 0;                  ///< Sessions that read it
    std::string changed;                    ///< YYYY-MM-DD
};

struct memory_t {
    std::filesystem::path dir;
    std::uintmax_t index_bytes = 0;         ///< `MEMORY.md`
    std::size_t sessions = 0;               ///< Transcripts next to the directory
    std::vector<memory_file_t> files;       ///< Most read first, then by name

    [[nodiscard]] auto bytes() const -> std::uintmax_t;
};

/// The memory files a transcript read, and its size and write time when it was scanned.
struct scan_t {
    std::uintmax_t size = 0;
    std::int64_t written = 0;               ///< Ticks of the file clock
    std::set<std::string> files;

    auto operator==(const scan_t&) const -> bool = default;
};

/// Scans of transcripts by path: a transcript is scanned again only once it changed.
using scan_cache_t = std::map<std::string, scan_t>;

/// The cache in `text`: a line per transcript, `path<TAB>size<TAB>written<TAB>file,file`.
[[nodiscard]] auto parse_scan_cache(std::string_view text) -> scan_cache_t;

[[nodiscard]] auto format_scan_cache(const scan_cache_t& cache) -> std::string;

/// `$XDG_CACHE_HOME/heimdall/memory-reads.tsv`, else under `~/.cache`.
[[nodiscard]] auto scan_cache_path() -> std::filesystem::path;

/// The front matter of a memory file's `text`; empty fields if it has none.
[[nodiscard]] auto parse_front_matter(std::string_view text) -> front_matter_t;

/// Tokens a text of `bytes` takes, roughly: four bytes a token.
[[nodiscard]] auto approx_tokens(std::uintmax_t bytes) -> std::uintmax_t;

/**
 * @brief   Where a memory read in many of `sessions` belongs instead: a
 *          reference in documentation or a skill, a project fact in the
 *          project's documentation, feedback in `CLAUDE.md`. Empty when it is
 *          read in fewer than 5 sessions or 15% of them, or is about the user.
 */
[[nodiscard]] auto promotion_hint(const memory_file_t& file, std::size_t sessions) -> std::string;

/**
 * @brief   The memory directories: `configured` (with `~` expanded), else
 *          every `<projects>/<slug>/memory` there is.
 */
[[nodiscard]] auto memory_dirs(const std::vector<std::string>& configured, const std::filesystem::path& projects)
    -> std::vector<std::filesystem::path>;

/**
 * @brief   The memory in `dir`, with the reads of the transcripts next to it:
 *          from `cached` for a transcript that did not change since, else
 *          scanned. Every transcript's scan goes into `scanned`.
 */
[[nodiscard]] auto load_memory(const std::filesystem::path& dir, const scan_cache_t& cached, scan_cache_t& scanned)
    -> memory_t;

/// `heimdall memory`: every directory's files, sizes, reads and hints.
[[nodiscard]] auto render_memory(const style_t& style, const std::vector<memory_t>& memories, std::size_t width)
    -> std::string;

/// The dashboard's line: files, size, tokens, and how many to promote.
[[nodiscard]] auto memory_summary(const style_t& style, const std::vector<memory_t>& memories) -> std::string;

} // namespace heimdall
