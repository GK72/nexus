/**
 * Part of Nexus Library
 *
 * @brief   Unit tests of the tasks: the plan, the repositories of projects, the views,
 *          approval, shelving and tokens.
 *
 * @author      Gábor Krisztián Girhiny
 * @created     2026-11-14
 * @updated     2026-11-14
 * @reviewed    none
 * SPDX-License-Identifier: Unlicense
 */

#include "heimdall/tasks.hpp"

#include <gtest/gtest.h>

#include <string_view>

namespace {

// app: base unblocks left and right, both unblock top (a diamond); top also waits on lib/core.
// lib: core, open; old, done.
constexpr std::string_view Plan = R"({
  "tasks": [
    {"id": "app/x/base",  "project": "app", "kind": "debt",    "title": "Base",  "status": "open", "state": "ready",
     "added": "2026-11-01", "closed": null, "note": "", "refs": "", "original": 2, "current": 3, "spent": 1, "moves": 2},
    {"id": "app/x/left",  "project": "app", "kind": "feature", "title": "Left",  "status": "open", "state": "blocked",
     "added": "2026-11-01", "closed": null, "note": "", "refs": "", "original": 1, "current": 1, "spent": 0},
    {"id": "app/x/right", "project": "app", "kind": "feature", "title": "Right", "status": "open", "state": "blocked",
     "added": "2026-11-01", "closed": null, "note": "", "refs": "", "original": 1, "current": 1, "spent": 0},
    {"id": "app/x/top",   "project": "app", "kind": "feature", "title": "Top",   "status": "open", "state": "blocked",
     "added": "2026-11-01", "closed": null, "note": "line one\nline two", "refs": "a.cpp:1",
     "original": 0.5, "current": 0.5, "spent": 0},
    {"id": "lib/core",    "project": "lib", "kind": "bug",     "title": "Core",  "status": "open", "state": "ready",
     "added": "2026-11-02", "closed": null, "note": "", "refs": "", "original": 4, "current": 4, "spent": 0},
    {"id": "lib/old",     "project": "lib", "kind": "debt",    "title": "Old",   "status": "done", "state": "done",
     "added": "2026-10-01", "closed": "2026-10-02", "note": "", "refs": "", "original": 1, "current": 1, "spent": 1}
  ],
  "deps": [
    {"task": "app/x/left",  "needs": "app/x/base"},
    {"task": "app/x/right", "needs": "app/x/base"},
    {"task": "app/x/top",   "needs": "app/x/left"},
    {"task": "app/x/top",   "needs": "app/x/right"},
    {"task": "app/x/top",   "needs": "lib/core"},
    {"task": "lib/core",    "needs": "lib/old"}
  ]
})";

auto plan() -> heimdall::plan_t {
    auto doc = nova::dyn_value::parse(Plan, nova::dyn_value::format_e::json);
    EXPECT_TRUE(doc.has_value());
    auto parsed = heimdall::parse_plan(*doc);
    EXPECT_TRUE(parsed.has_value());
    return *parsed;
}

const auto Plain = heimdall::style_t::detect(heimdall::color_mode::never);

auto view() -> heimdall::tasks_view {
    return { .width = 72, .repos = { { "app", "/src/app" } } };
}

} // namespace

TEST(Tasks, ParsePlan) {
    const auto parsed = plan();
    ASSERT_EQ(parsed.tasks.size(), 6);
    const auto* base = parsed.find("app/x/base");
    ASSERT_NE(base, nullptr);
    EXPECT_EQ(base->name(), "x/base");
    EXPECT_EQ(base->left(), 2.0);
    EXPECT_EQ(base->moves, 2);
    EXPECT_FALSE(base->closed);
    EXPECT_EQ(parsed.find("lib/old")->closed, "2026-10-02");
    EXPECT_EQ(parsed.needs("app/x/top").size(), 3);
    EXPECT_EQ(parsed.needed_by("app/x/base").size(), 2);
}

TEST(Tasks, ParsePlanRefusesAMissingField) {
    auto doc = nova::dyn_value::parse(R"({"tasks": [{"id": "a/b"}], "deps": []})", nova::dyn_value::format_e::json);
    ASSERT_TRUE(doc.has_value());
    EXPECT_FALSE(heimdall::parse_plan(*doc).has_value());
}

TEST(Tasks, ProjectReposByNameOrConfigured) {
    const auto parsed = plan();
    const std::vector<std::filesystem::path> repos { "/src/app", "/src/lib-cpp" };
    EXPECT_EQ(heimdall::project_repos(parsed, repos, {}),
              (std::map<std::string, std::string>{ { "app", "/src/app" } }));
    EXPECT_EQ(heimdall::project_repos(parsed, repos, { { "lib", "lib-cpp" } }),
              (std::map<std::string, std::string>{ { "app", "/src/app" }, { "lib", "/src/lib-cpp" } }));
    EXPECT_EQ(heimdall::project_repos(parsed, repos, { { "lib", "/elsewhere/lib" } }).at("lib"), "/elsewhere/lib");
}

TEST(Tasks, OverviewIsATreePerProject) {
    EXPECT_EQ(heimdall::render_tasks(Plain, plan(), "", view()),
        "\n"
        "  Tasks                                    5 open · 2 projects · 8.5h left\n"
        "\n"
        "  ── app ───────────── /src/app · 4 open · 1 ready · 3 blocked · 4.5h left\n"
        "  x/base       debt       2h  ready    Base\n"
        "  ├─ x/left    feature    1h  blocked  Left\n"
        "  │  └─ x/top  feature  0.5h  blocked  needs lib/core Top\n"
        "  └─ x/right   feature    1h  blocked  Right\n"
        "     └─ x/top  ↑ above\n"
        "\n"
        "  ── lib ────────────────────── no repository · 1 open · 1 ready · 4h left\n"
        "  core         bug        4h  ready    Core\n");
}

TEST(Tasks, OverviewUnderAPrefix) {
    const auto text = heimdall::render_tasks(Plain, plan(), "lib", view());
    EXPECT_NE(text.find("── lib"), std::string::npos);
    EXPECT_EQ(text.find("── app"), std::string::npos);
    EXPECT_NE(heimdall::render_tasks(Plain, plan(), "app/x/b", view()).find("no open task under app/x/b"),
              std::string::npos);
}

TEST(Tasks, DetailShowsWhatItWaitsOnAndUnblocks) {
    const auto parsed = plan();
    EXPECT_EQ(heimdall::render_task(Plain, parsed, *parsed.find("app/x/left"), view()),
        "\n"
        "  app/x/left                                             feature · blocked\n"
        "  Left\n"
        "\n"
        "  estimate 1h · spent 0h · left 1h\n"
        "  added 2026-11-01 · priority 2 · by the author\n"
        "  repository /src/app\n"
        "\n"
        "  ── waits on ──────────────────────────── 1 open · 2h before it can start\n"
        "  x/base  debt       2h  ready    Base\n"
        "\n"
        "  ── unblocks ───────────────────────────────────────────────────── 1 open\n"
        "  x/top  feature  0.5h  blocked  Top\n");
}

TEST(Tasks, DetailFollowsTheChainAcrossProjects) {
    const auto parsed = plan();
    const auto text = heimdall::render_task(Plain, parsed, *parsed.find("app/x/top"), view());
    EXPECT_NE(text.find("4 open · 8h before it can start"), std::string::npos) << text;
    EXPECT_NE(text.find("└─ lib/old"), std::string::npos) << text;
    EXPECT_NE(text.find("── note"), std::string::npos);
    EXPECT_NE(text.find("  line one\n  line two\n"), std::string::npos);
    EXPECT_NE(text.find("  a.cpp:1\n"), std::string::npos);
}

TEST(Tasks, DetailOfAClosedTask) {
    const auto parsed = plan();
    const auto text = heimdall::render_task(Plain, parsed, *parsed.find("lib/old"), view());
    EXPECT_NE(text.find("added 2026-10-01 · done 2026-10-02"), std::string::npos) << text;
    EXPECT_EQ(text.find("left"), std::string::npos) << text;
}

namespace {

// norn: a task Claude proposed, one it added that was approved (priority 1, with a session),
// and one of the author's; aya is shelved.
constexpr std::string_view Proposals = R"({
  "tasks": [
    {"id": "norn/a", "project": "norn", "kind": "feature", "title": "A", "status": "open", "state": "proposed",
     "original": 1, "current": 1, "spent": 0, "source": "claude", "approved": false, "tokens": 40000},
    {"id": "norn/b", "project": "norn", "kind": "feature", "title": "B", "status": "open", "state": "ready",
     "original": 2, "current": 2, "spent": 0, "source": "claude", "approved": true, "priority": 1, "tokens": 60000},
    {"id": "norn/c", "project": "norn", "kind": "debt",    "title": "C", "status": "open", "state": "ready",
     "original": 1, "current": 1, "spent": 0},
    {"id": "aya/x",  "project": "aya",  "kind": "feature", "title": "X", "status": "open", "state": "ready",
     "original": 3, "current": 3, "spent": 0, "shelved": true}
  ],
  "deps": [],
  "sessions": [{"task": "norn/b", "session": "s1"}, {"task": "norn/c", "session": "s1"}]
})";

auto proposals() -> heimdall::plan_t {
    auto doc = nova::dyn_value::parse(Proposals, nova::dyn_value::format_e::json);
    EXPECT_TRUE(doc.has_value());
    auto parsed = heimdall::parse_plan(*doc);
    EXPECT_TRUE(parsed.has_value());
    heimdall::attribute_tokens(*parsed, { { "s1", heimdall::usage_t { .input = 10, .cache_creation = 9'990,
                                                                       .cache_read = 500'000, .output = 10'000 } } });
    return *parsed;
}

} // namespace

TEST(Tasks, ParsesApprovalPriorityTokensAndSessions) {
    const auto parsed = proposals();
    EXPECT_TRUE(parsed.find("norn/a")->proposed());
    EXPECT_TRUE(parsed.find("norn/b")->by_claude());
    EXPECT_EQ(parsed.find("norn/b")->priority, 1);
    EXPECT_EQ(parsed.find("norn/c")->priority, 2);
    EXPECT_TRUE(parsed.find("aya/x")->shelved);
    EXPECT_EQ(parsed.session_ids(), (std::set<std::string>{ "s1" }));
    // 20k spent in s1, without cache reads, split between the two tasks linked to it.
    EXPECT_EQ(parsed.find("norn/b")->tokens_spent, 10'000);
    EXPECT_EQ(parsed.find("norn/c")->tokens_spent, 10'000);
    EXPECT_EQ(parsed.find("norn/a")->tokens_spent, 0);
}

TEST(Tasks, ProposedFirstShelvedOnlyCounted) {
    EXPECT_EQ(heimdall::render_tasks(Plain, proposals(), "", view()),
        "\n"
        "  Tasks                            2 open · 1 project · 3h left · ~50k tok\n"
        "                           1 awaiting approval · shelved: aya (1 task, 3h)\n"
        "\n"
        "  ── awaiting approval ───────────────────────── norn.py task approve <id>\n"
        "   ✦ norn/a  feature    1h      ~40k  proposed  A\n"
        "\n"
        "  ── norn ────────── no repository · 2 open · 2 ready · 3h left · ~50k tok\n"
        "  1✦ b       feature    2h   10k/60k  ready    B\n"
        "     c       debt       1h     10k/?  ready    C\n");
}

TEST(Tasks, ShelvedShownWithAllOrItsPrefix) {
    auto all = view();
    all.shelved = true;
    EXPECT_NE(heimdall::render_tasks(Plain, proposals(), "", all).find("── aya"), std::string::npos);
    EXPECT_NE(heimdall::render_tasks(Plain, proposals(), "aya", view()).find("── aya"), std::string::npos);
}

TEST(Tasks, DetailShowsTokensAndApproval) {
    const auto parsed = proposals();
    const auto text = heimdall::render_task(Plain, parsed, *parsed.find("norn/b"), view());
    EXPECT_NE(text.find("tokens ~60k · spent 10k · sessions 1"), std::string::npos) << text;
    EXPECT_NE(text.find("priority 1 · by Claude"), std::string::npos) << text;
    const auto proposed = heimdall::render_task(Plain, parsed, *parsed.find("norn/a"), view());
    EXPECT_NE(proposed.find("awaiting approval"), std::string::npos) << proposed;
}

TEST(Tasks, SummaryLeavesShelvedOutAndCountsProposed) {
    EXPECT_EQ(heimdall::tasks_summary(Plain, proposals()),
              "2 open · 3h left · ~50k tok · 1 awaiting approval  heimdall tasks");
}
