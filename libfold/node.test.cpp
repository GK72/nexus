#include <libfold/node.hpp>
#include <libfold/sum_count.test.hpp>

#include <gtest/gtest.h>

#include <utility>

namespace {

using node     = fold::node<sum_count>;
using envelope = fold::envelope<sum_count>;
using partial  = sum_count::partial;

constexpr fold::window_config cfg { .width = 10, .allowed_lateness = 0 };

auto value_of(const envelope& env, fold::window_id w) -> partial {
    for (const auto& [id, p] : env.windows) {
        if (id == w) {
            return p;
        }
    }
    return {};
}

} // namespace

TEST(WindowConfig, Boundaries) {
    EXPECT_EQ(cfg.window_of(0), 0);
    EXPECT_EQ(cfg.window_of(9), 0);
    EXPECT_EQ(cfg.window_of(10), 1);
    EXPECT_EQ(cfg.window_of(-1), -1);
    EXPECT_EQ(cfg.closes_at(0), 10);
}

TEST(Node, FinalizesOnlyAfterWatermarkPasses) {
    node n(0, cfg);
    n.ingest(1, 5);
    n.ingest(9, 3);
    n.ingest(12, 100);

    EXPECT_TRUE(n.flush(true).finalized.empty());   // no watermark yet

    n.advance(9);
    EXPECT_TRUE(n.flush(true).finalized.empty());   // window [0,10) still open

    n.advance(10);
    auto out = n.flush(true);
    ASSERT_EQ(out.finalized.size(), 1U);
    EXPECT_EQ(out.finalized[0].first, 0);
    EXPECT_EQ(out.finalized[0].second, (partial { .sum = 8, .count = 2 }));
    EXPECT_EQ(n.open_windows(), 1U);
}

TEST(Node, LateEventIsDroppedAndCounted) {
    node n(0, cfg);
    n.advance(20);
    n.flush(true);

    EXPECT_FALSE(n.ingest(5, 1));
    EXPECT_TRUE(n.ingest(25, 1));
    EXPECT_EQ(n.stats().late_events, 1U);
}

TEST(Node, AllowedLatenessKeepsWindowOpen) {
    node n(0, { .width = 10, .allowed_lateness = 5 });
    n.advance(12);
    n.flush(true);
    EXPECT_TRUE(n.ingest(3, 1));    // within lateness

    n.advance(15);
    auto out = n.flush(true);
    ASSERT_EQ(out.finalized.size(), 1U);
    EXPECT_EQ(out.finalized[0].second.count, 1);
}

TEST(Node, EnvelopeCarriesCumulativeValueOfChangedWindowsOnly) {
    node n(7, cfg);
    n.ingest(1, 1);
    n.ingest(11, 10);
    auto first = n.flush(false).out;
    EXPECT_EQ(first.source, 7U);
    EXPECT_EQ(first.windows.size(), 2U);

    n.ingest(2, 1);
    auto second = n.flush(false).out;
    ASSERT_EQ(second.windows.size(), 1U);
    EXPECT_EQ(value_of(second, 0), (partial { .sum = 2, .count = 2 }));   // cumulative, not delta
    EXPECT_GT(second.epoch, first.epoch);

    EXPECT_TRUE(n.flush(false).out.windows.empty());
}

TEST(Node, DuplicateEnvelopeIsIgnored) {
    node child(1, cfg);
    node parent(0, cfg);
    parent.attach(1);

    child.ingest(1, 4);
    const auto env = child.flush(false).out;

    EXPECT_TRUE(parent.receive(env));
    EXPECT_FALSE(parent.receive(env));
    EXPECT_EQ(parent.stats().duplicate_envelopes, 1U);

    auto out = parent.flush(false).out;
    EXPECT_EQ(value_of(out, 0), (partial { .sum = 4, .count = 1 }));
}

TEST(Node, ResendWithNewerEpochReplacesInsteadOfAdding) {
    node parent(0, cfg);
    parent.attach(1);

    const auto value = partial { .sum = 4, .count = 1 };
    parent.receive({ .source = 1, .epoch = 1, .watermark = std::nullopt, .windows = { { 0, value } } });
    parent.receive({ .source = 1, .epoch = 2, .watermark = std::nullopt, .windows = { { 0, value } } });

    EXPECT_EQ(value_of(parent.flush(false).out, 0), value);
}

TEST(Node, RejectsUnknownSource) {
    node parent(0, cfg);
    EXPECT_FALSE(parent.receive({ .source = 9, .epoch = 1, .watermark = 0, .windows = {} }));
    EXPECT_EQ(parent.stats().rejected_envelopes, 1U);
}

TEST(Node, WatermarkIsHeldUntilEveryChildReports) {
    node parent(0, cfg);
    parent.attach(1);
    parent.attach(2);

    parent.receive({ .source = 1, .epoch = 1, .watermark = 100, .windows = {} });
    parent.flush(false);
    EXPECT_FALSE(parent.watermark().has_value());

    parent.receive({ .source = 2, .epoch = 1, .watermark = 50, .windows = {} });
    parent.flush(false);
    EXPECT_EQ(parent.watermark(), 50);

    parent.receive({ .source = 2, .epoch = 2, .watermark = 30, .windows = {} });
    parent.flush(false);
    EXPECT_EQ(parent.watermark(), 50);   // never moves backwards
}

TEST(Node, DetachKeepsFinalWindowsAndDropsOpenOnes) {
    node parent(0, cfg);
    parent.attach(1);
    parent.attach(2);
    parent.receive({ .source = 2, .epoch = 1, .watermark = 0, .windows = {} });   // holds parent at 0

    parent.receive({ .source = 1, .epoch = 1, .watermark = 10, .windows = {
        { 0, { .sum = 1, .count = 1 } },     // final for child 1 (its watermark is 10)
        { 1, { .sum = 5, .count = 1 } },     // still open for child 1
    } });
    parent.flush(false);

    parent.detach(1);
    auto out = parent.flush(false).out;

    // The shrunken window is re-sent so ancestors drop the moved contribution.
    ASSERT_EQ(out.windows.size(), 1U);
    EXPECT_EQ(out.windows[0].first, 1);
    EXPECT_EQ(out.windows[0].second, partial {});

    // Window 0 survives and is finalized once the parent's watermark allows.
    parent.receive({ .source = 2, .epoch = 2, .watermark = 10, .windows = {} });
    auto done = parent.flush(true);
    ASSERT_EQ(done.finalized.size(), 1U);
    EXPECT_EQ(done.finalized[0].second, (partial { .sum = 1, .count = 1 }));
}

TEST(Node, PartialForClosedWindowIsCountedAsLate) {
    node parent(0, cfg);
    parent.attach(1);
    parent.receive({ .source = 1, .epoch = 1, .watermark = 20, .windows = {} });
    parent.flush(true);

    parent.receive({ .source = 1, .epoch = 2, .watermark = 20, .windows = { { 0, { .sum = 1, .count = 1 } } } });
    EXPECT_EQ(parent.stats().late_partials, 1U);
}
