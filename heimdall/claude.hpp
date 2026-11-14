/**
 * Part of Nexus Library
 *
 * @brief   What Claude Code keeps locally: the transcripts of its sessions, the
 *          tokens and time they used and the memory files they read.
 *
 * A session's transcript is `<projects>/<slug>/<session>.jsonl`, one JSON
 * object per line; its subagents write `<slug>/<session>/subagents/<agent>.jsonl`.
 *
 * @author      Gábor Krisztián Girhiny
 * @created     2026-11-14
 * @updated     2026-11-14
 * @status      experimental
 * @reviewed    none
 * SPDX-License-Identifier: Unlicense
 */

#pragma once

#include <cstdint>
#include <filesystem>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace heimdall {

/// Tokens of the model's answers in a transcript, and the time Claude took.
struct usage_t {
    std::int64_t input = 0;
    std::int64_t cache_creation = 0;
    std::int64_t cache_read = 0;
    std::int64_t output = 0;
    std::int64_t wall_ms = 0;               ///< The turns, from the prompt to the answer's end
    std::int64_t api_ms = 0;                ///< The requests, from sending to the answer's last part

    /// The tokens a session spent: new input, cache writes and output; cache reads are left out.
    [[nodiscard]] auto spent() const -> std::int64_t { return input + cache_creation + output; }
    auto operator+=(const usage_t& other) -> usage_t&;
};

/**
 * @brief   The usage of the answers in the transcript `jsonl`, each answer
 *          once: a transcript repeats an answer's usage on every part of it.
 *
 * The wall time is the sum of the turns' `turn_duration`; the API time, of each
 * `api-request` up to the last part of its answer (by `requestRef`), so the time
 * of the tools and of the author's replies is left out of it.
 */
[[nodiscard]] auto transcript_usage(std::string_view jsonl) -> usage_t;

/**
 * @brief   The memory files the transcript `jsonl` read: with the Read tool,
 *          in a shell command that names a `memory` directory (every `.md`
 *          file it names counts), or attached to a prompt when Claude Code
 *          recalled them. The names of the files, the index left out.
 */
[[nodiscard]] auto memory_reads(std::string_view jsonl) -> std::set<std::string>;

/// The default directory of Claude Code's projects: `~/.claude/projects`.
[[nodiscard]] auto claude_projects_dir() -> std::filesystem::path;

/**
 * @brief   The usage of each of `sessions`, read from their transcripts and
 *          their subagents' under `projects`; a session without a transcript
 *          is left out. Subagents add tokens and API time; their wall time is
 *          within the session's turns.
 */
[[nodiscard]] auto session_usage(const std::filesystem::path& projects, const std::set<std::string>& sessions)
    -> std::map<std::string, usage_t>;

/// Tokens as `850`, `60k`, `1.2k` or `1.4M`.
[[nodiscard]] auto tokens_text(std::int64_t tokens) -> std::string;

/// A duration as `45s`, `12m` or `2h05m`.
[[nodiscard]] auto duration_text(std::int64_t ms) -> std::string;

/// Milliseconds since the epoch of a transcript's `2026-11-14T14:30:46.426Z`; nothing if malformed.
[[nodiscard]] auto timestamp_ms(std::string_view text) -> std::optional<std::int64_t>;

/// `path` as text; empty if it cannot be read.
[[nodiscard]] auto read_text(const std::filesystem::path& path) -> std::string;

/// The `*.jsonl` files directly in `dir`, by name.
[[nodiscard]] auto transcripts_in(const std::filesystem::path& dir) -> std::vector<std::filesystem::path>;

} // namespace heimdall
