/**
 * Part of Nexus Library
 *
 * @brief   Unit tests of Claude's memory: front matter, hints, the views.
 *
 * @author      Gábor Krisztián Girhiny
 * @created     2026-11-14
 * @updated     2026-11-14
 * @reviewed    none
 * SPDX-License-Identifier: Unlicense
 */

#include "heimdall/memory.hpp"

#include <gtest/gtest.h>

namespace {

const auto Plain = heimdall::style_t::detect(heimdall::color_mode::never);

auto file(std::string name, std::string type, std::size_t reads, std::uintmax_t bytes = 2'000)
    -> heimdall::memory_file_t
{
    return { .file = std::move(name), .about = { .type = std::move(type) }, .bytes = bytes, .reads = reads,
             .changed = "2026-11-13" };
}

auto memory() -> heimdall::memory_t {
    return {
        .dir = "/m/memory",
        .index_bytes = 800,
        .sessions = 40,
        .files = { file("norn.md", "reference", 10, 3'000), file("style.md", "feedback", 6), file("who.md", "user", 9),
                   file("rare.md", "project", 1) },
    };
}

} // namespace

TEST(Memory, FrontMatter) {
    const auto about = heimdall::parse_front_matter(
        "---\nname: norn-service\ndescription: \"the task service: norn.py\"\nmetadata:\n  type: reference\n---\n\nBody\ntype: no\n");
    EXPECT_EQ(about.name, "norn-service");
    EXPECT_EQ(about.description, "the task service: norn.py");
    EXPECT_EQ(about.type, "reference");
    EXPECT_EQ(heimdall::parse_front_matter("No front matter\ntype: x\n").type, "");
}

TEST(Memory, PromotionHintByTypeWhenReadOften) {
    const auto sessions = 40;
    EXPECT_EQ(heimdall::promotion_hint(file("a", "reference", 6), sessions), "→ doc or skill?");
    EXPECT_EQ(heimdall::promotion_hint(file("a", "project", 6), sessions), "→ project doc?");
    EXPECT_EQ(heimdall::promotion_hint(file("a", "feedback", 6), sessions), "→ CLAUDE.md?");
    EXPECT_EQ(heimdall::promotion_hint(file("a", "user", 30), sessions), "");
    EXPECT_EQ(heimdall::promotion_hint(file("a", "reference", 5), sessions), "") << "under 15% of the sessions";
    EXPECT_EQ(heimdall::promotion_hint(file("a", "reference", 4), 10), "") << "under 5 sessions";
}

TEST(Memory, ApproxTokens) {
    EXPECT_EQ(heimdall::approx_tokens(0), 0);
    EXPECT_EQ(heimdall::approx_tokens(4'001), 1'001);
}

TEST(Memory, Overview) {
    EXPECT_EQ(heimdall::render_memory(Plain, { memory() }, 72),
        "\n"
        "  Memory                                      4 files · 9.8 KB · ~2.5k tok\n"
        "                               index 800 B (~200 tok) loaded every session\n"
        "\n"
        "  ── /m/memory ───────────────────────────────────── 4 files · 40 sessions\n"
        "  FILE      TYPE         SIZE   ~TOK READ IN  CHANGED     HINT\n"
        "  norn.md   reference  3.0 KB    750   10/40  2026-11-13  → doc or skill?\n"
        "  style.md  feedback   2.0 KB    500    6/40  2026-11-13  → CLAUDE.md?\n"
        "  who.md    user       2.0 KB    500    9/40  2026-11-13\n"
        "  rare.md   project    2.0 KB    500    1/40  2026-11-13\n");
}

TEST(Memory, Summary) {
    EXPECT_EQ(heimdall::memory_summary(Plain, { memory() }),
              "4 files · 9.8 KB · ~2.5k tok · 2 to promote  heimdall memory");
}

TEST(Memory, ScanCacheRoundTrip) {
    const heimdall::scan_cache_t cache {
        { "/p/a.jsonl", { .size = 10, .written = 123, .files = { "x.md", "y.md" } } },
        { "/p/b.jsonl", { .size = 0, .written = -5, .files = {} } },
    };
    const auto text = heimdall::format_scan_cache(cache);
    EXPECT_EQ(text, "/p/a.jsonl\t10\t123\tx.md,y.md\n/p/b.jsonl\t0\t-5\t\n");
    EXPECT_EQ(heimdall::parse_scan_cache(text), cache);
    EXPECT_TRUE(heimdall::parse_scan_cache("broken line\n/p\tx\t1\t\n").empty());
}
