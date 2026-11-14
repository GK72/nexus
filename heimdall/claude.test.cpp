/**
 * Part of Nexus Library
 *
 * @brief   Unit tests of Claude's local data: tokens of transcripts, memory reads.
 *
 * @author      Gábor Krisztián Girhiny
 * @created     2026-11-14
 * @updated     2026-11-14
 * @reviewed    none
 * SPDX-License-Identifier: Unlicense
 */

#include "heimdall/claude.hpp"

#include <gtest/gtest.h>

#include <string_view>

namespace {

// Answer m1 in two parts (its usage repeated), answer m2, a user line, and a broken line.
constexpr std::string_view Transcript =
    R"({"type":"assistant","message":{"id":"m1","usage":{"input_tokens":2,"cache_creation_input_tokens":100,"cache_read_input_tokens":1000,"output_tokens":50},"content":[{"type":"thinking"}]}})" "\n"
    R"({"type":"assistant","message":{"id":"m1","usage":{"input_tokens":2,"cache_creation_input_tokens":100,"cache_read_input_tokens":1000,"output_tokens":50},"content":[{"type":"tool_use","name":"Read","input":{"file_path":"/home/u/.claude/projects/p/memory/norn-service.md"}}]}})" "\n"
    R"({"type":"assistant","message":{"id":"m2","usage":{"input_tokens":1,"cache_creation_input_tokens":0,"cache_read_input_tokens":2000,"output_tokens":7},"content":[{"type":"tool_use","name":"Read","input":{"file_path":"/repo/memory/notes.txt"}},{"type":"tool_use","name":"Write","input":{"file_path":"/home/u/.claude/projects/p/memory/other.md"}}]}})" "\n"
    R"({"type":"user","message":{"content":"the usage of memory/ in a Read"}})" "\n"
    R"({"type":"attachment","attachment":{"type":"file","filename":"/home/u/.claude/projects/p/memory/recalled.md"}})" "\n"
    R"({"type":"attachment","attachment":{"type":"file","filename":"/home/u/.claude/projects/p/memory/MEMORY.md"}})" "\n"
    R"({"type":"attachment","attachment":{"type":"edited_text_file","filename":"/home/u/.claude/projects/p/memory/edited.md"}})" "\n"
    R"({"type":"assistant","message":{"id":"m3","content":[{"type":"tool_use","name":"Bash","input":{"command":"cd ~/.claude/projects/p/memory; cat a.md \"b.md\" MEMORY.md | head; cat ~/repos/x/README.md"}}]}})" "\n"
    R"({"type":"assistant","message":{"id":"m4","content":[{"type":"tool_use","name":"Bash","input":{"command":"cat docs/c.md"}}]}})" "\n"
    R"({"type":"assistant", "usage" broken)" "\n";

// Two turns: request r1 answered in two parts (3s), r2 (1.5s), and r3 that never got an answer.
constexpr std::string_view Timed =
    R"({"type":"api-request","id":"r1","timestamp":"2026-11-14T14:30:46.000Z"})" "\n"
    R"({"type":"assistant","requestRef":"r1","timestamp":"2026-11-14T14:30:48.000Z","message":{"id":"m1","usage":{"output_tokens":1}}})" "\n"
    R"({"type":"assistant","requestRef":"r1","timestamp":"2026-11-14T14:30:49.000Z","message":{"id":"m1","usage":{"output_tokens":1}}})" "\n"
    R"({"type":"api-request","id":"r2","timestamp":"2026-11-14T14:30:59.500Z"})" "\n"
    R"({"type":"assistant","requestRef":"r2","timestamp":"2026-11-14T14:31:01.000Z","message":{"id":"m2","usage":{"output_tokens":1}}})" "\n"
    R"({"type":"api-request","id":"r3","timestamp":"2026-11-14T14:32:00.000Z"})" "\n"
    R"({"type":"api-request-blob","id":"r1","timestamp":"2026-11-14T14:00:00.000Z"})" "\n"
    R"({"type":"system","subtype":"turn_duration","durationMs":20000})" "\n"
    R"({"type":"system","subtype":"turn_duration","durationMs":5000})" "\n";

} // namespace

TEST(Claude, TimesAreTurnsAndAnsweredRequests) {
    const auto usage = heimdall::transcript_usage(Timed);
    EXPECT_EQ(usage.wall_ms, 25'000);
    EXPECT_EQ(usage.api_ms, 4'500);
    EXPECT_EQ(usage.output, 2);
}

TEST(Claude, TimestampMs) {
    EXPECT_EQ(heimdall::timestamp_ms("1970-01-01T00:00:01.250Z"), 1'250);
    EXPECT_EQ(heimdall::timestamp_ms("1970-01-02T00:00:00Z"), 86'400'000);
    EXPECT_EQ(heimdall::timestamp_ms("2026-11-14T14:30:46.4Z").value() % 1'000, 400);
    EXPECT_FALSE(heimdall::timestamp_ms("2026-13-14T14:30:46Z"));
    EXPECT_FALSE(heimdall::timestamp_ms("yesterday"));
}

TEST(Claude, DurationText) {
    EXPECT_EQ(heimdall::duration_text(0), "0s");
    EXPECT_EQ(heimdall::duration_text(44'600), "45s");
    EXPECT_EQ(heimdall::duration_text(12 * 60'000), "12m");
    EXPECT_EQ(heimdall::duration_text(125 * 60'000), "2h05m");
}

TEST(Claude, UsageCountsEachAnswerOnce) {
    const auto usage = heimdall::transcript_usage(Transcript);
    EXPECT_EQ(usage.input, 3);
    EXPECT_EQ(usage.cache_creation, 100);
    EXPECT_EQ(usage.cache_read, 3000);
    EXPECT_EQ(usage.output, 57);
    EXPECT_EQ(usage.spent(), 160);
}

TEST(Claude, MemoryReadsAreReadCallsAndRecalledFiles) {
    EXPECT_EQ(heimdall::memory_reads(Transcript), (std::set<std::string>{ "a.md", "b.md", "norn-service.md", "recalled.md" }));
}

TEST(Claude, TokensText) {
    EXPECT_EQ(heimdall::tokens_text(850), "850");
    EXPECT_EQ(heimdall::tokens_text(1'234), "1.2k");
    EXPECT_EQ(heimdall::tokens_text(60'000), "60k");
    EXPECT_EQ(heimdall::tokens_text(123'456), "123k");
    EXPECT_EQ(heimdall::tokens_text(1'400'000), "1.4M");
}
