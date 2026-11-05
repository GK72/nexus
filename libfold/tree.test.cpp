#include <libfold/player_analytics.hpp>
#include <libfold/sum_count.test.hpp>
#include <libfold/tree.hpp>

#include <gtest/gtest.h>

#include <map>
#include <random>
#include <stdexcept>
#include <vector>

namespace {

constexpr fold::window_config cfg { .width = 20, .allowed_lateness = 3 };

struct event {
    fold::timestamp ts;
    std::int64_t value;
};

/**
 * Feed the same events into a random tree (random leaf per event, random
 * re-parenting while running) and into a single node. Exact aggregations
 * must agree bit for bit.
 */
auto run_random(std::uint64_t seed, bool with_moves)
    -> std::pair<std::map<fold::window_id, sum_count::partial>, std::map<fold::window_id, sum_count::partial>>
{
    std::mt19937_64 rng(seed);
    std::map<fold::window_id, sum_count::partial> hier_out;
    std::map<fold::window_id, sum_count::partial> flat_out;

    fold::tree<sum_count> hier(cfg, [&](auto w, const auto& r) { hier_out[w] = r; });
    fold::tree<sum_count> flat(cfg, [&](auto w, const auto& r) { flat_out[w] = r; });

    std::uniform_int_distribution<std::size_t> leaves(1, 300);
    std::uniform_int_distribution<std::size_t> fan_in(2, 6);
    const auto shape  = fold::build(hier, leaves(rng), [&](std::size_t) { return fan_in(rng); });
    const auto single = flat.add_node();

    std::uniform_int_distribution<std::size_t> any_leaf(0, shape.leaves.size() - 1);
    std::uniform_int_distribution<fold::node_id> any_node(0, static_cast<fold::node_id>(hier.size() - 1));
    std::uniform_int_distribution<std::int64_t> value(-50, 1000);
    std::bernoulli_distribution do_move(0.5);

    constexpr fold::timestamp end  = 200;
    constexpr fold::timestamp step = 5;

    for (fold::timestamp now = 0; now < end; now += step) {
        for (int i = 0; i < 500; ++i) {
            const auto ts = now + static_cast<fold::timestamp>(i % step);
            const auto v  = value(rng);
            hier.at(shape.leaves[any_leaf(rng)]).ingest(ts, v);
            flat.at(single).ingest(ts, v);
        }

        for (auto leaf : shape.leaves) { hier.at(leaf).advance(now + step); }
        flat.at(single).advance(now + step);

        if (with_moves && do_move(rng)) {
            const auto child  = any_node(rng);
            const auto target = any_node(rng);
            if (hier.parent_of(child)) {
                try {
                    hier.move(child, target);
                }
                catch (const std::logic_error&) {
                    // target inside child's subtree: skip
                }
            }
        }

        hier.tick();
        flat.tick();
    }

    for (auto leaf : shape.leaves) { hier.at(leaf).advance(end * 2); }
    flat.at(single).advance(end * 2);
    hier.tick();
    flat.tick();

    EXPECT_EQ(hier.total_stats().late_partials, 0U);
    EXPECT_EQ(hier.total_stats().late_events, 0U);
    return { hier_out, flat_out };
}

} // namespace

TEST(Tree, BuildsLayersFromFanIn) {
    fold::tree<sum_count> t(cfg, [](auto, const auto&) {});
    const auto shape = fold::build(t, 16, [](std::size_t) { return 2; });
    EXPECT_EQ(shape.leaves.size(), 16U);
    EXPECT_EQ(shape.layers, 5U);
    EXPECT_EQ(t.depth(), 5U);
    EXPECT_EQ(t.size(), 31U);
    EXPECT_EQ(t.roots(), std::vector<fold::node_id> { shape.root });
}

TEST(Tree, SingleLeafIsRoot) {
    fold::tree<sum_count> t(cfg, [](auto, const auto&) {});
    const auto shape = fold::build(t, 1, [](std::size_t) { return 4; });
    EXPECT_EQ(shape.root, shape.leaves.front());
    EXPECT_EQ(t.depth(), 1U);
}

TEST(Tree, RejectsCycles) {
    fold::tree<sum_count> t(cfg, [](auto, const auto&) {});
    const auto a = t.add_node();
    const auto b = t.add_node();
    t.attach(b, a);
    EXPECT_THROW(t.attach(a, b), std::logic_error);
    EXPECT_THROW(t.move(a, b), std::logic_error);
    EXPECT_THROW(t.attach(b, a), std::logic_error);   // already has a parent
}

TEST(Tree, AnyShapeMatchesSingleNode) {
    for (std::uint64_t seed = 1; seed <= 10; ++seed) {
        const auto [hier, flat] = run_random(seed, false);
        EXPECT_FALSE(flat.empty());
        EXPECT_EQ(hier, flat) << "seed " << seed;
    }
}

TEST(Tree, ReparentingWhileRunningStaysExact) {
    for (std::uint64_t seed = 100; seed <= 120; ++seed) {
        const auto [hier, flat] = run_random(seed, true);
        EXPECT_EQ(hier, flat) << "seed " << seed;
    }
}

TEST(Tree, MoveAfterChildClosedWindowKeepsFinalContribution) {
    // root <- a <- {x, slow};  root <- b
    std::map<fold::window_id, sum_count::partial> out;
    fold::tree<sum_count> t({ .width = 10, .allowed_lateness = 0 }, [&](auto w, const auto& r) { out[w] = r; });
    const auto root = t.add_node();
    const auto a    = t.add_node();
    const auto b    = t.add_node();
    const auto x    = t.add_node();
    const auto slow = t.add_node();
    t.attach(a, root);
    t.attach(b, root);
    t.attach(x, a);
    t.attach(slow, a);

    t.at(x).ingest(1, 7);       // window 0
    t.at(x).ingest(12, 100);    // window 1
    t.at(x).advance(10);        // x closes window 0 ...
    t.at(slow).advance(5);      // ... but `a` is held back by `slow`
    t.at(b).advance(10);
    t.tick();

    t.move(x, b);               // window 0 is final at x and must stay in a; window 1 moves with x
    t.at(slow).ingest(6, 1);
    t.tick();

    for (auto n : { x, slow, b }) { t.at(n).advance(100); }
    t.tick();

    EXPECT_EQ(out[0], (sum_count::partial { .sum = 8, .count = 2 }));
    EXPECT_EQ(out[1], (sum_count::partial { .sum = 100, .count = 1 }));
    EXPECT_EQ(t.total_stats().late_partials, 0U);
}

TEST(Tree, EmptiedNodeDoesNotStallTheTree) {
    std::map<fold::window_id, sum_count::partial> out;
    fold::tree<sum_count> t({ .width = 10, .allowed_lateness = 0 }, [&](auto w, const auto& r) { out[w] = r; });
    const auto root = t.add_node();
    const auto a    = t.add_node();
    const auto b    = t.add_node();
    const auto leaf = t.add_node();
    t.attach(a, root);
    t.attach(b, root);
    t.attach(leaf, a);

    t.at(leaf).ingest(1, 3);
    t.at(leaf).advance(5);
    t.tick();

    t.move(leaf, b);            // `a` now has no source and no children
    t.at(leaf).advance(50);
    t.tick();

    ASSERT_EQ(out.count(0), 1U);
    EXPECT_EQ(out[0], (sum_count::partial { .sum = 3, .count = 1 }));
}

TEST(Tree, PlayerAnalyticsExactPartsAreTopologyIndependent) {
    using game::event_kind;
    using analytics = game::player_analytics;

    std::mt19937_64 rng(7);
    std::vector<std::pair<fold::timestamp, game::player_event>> events;
    std::uniform_int_distribution<std::uint64_t> player(1, 3'000);
    std::uniform_int_distribution<std::uint32_t> score(0, 2'000);
    std::uniform_int_distribution<int> kind(0, 3);
    for (fold::timestamp ts = 0; ts < 120; ++ts) {
        for (int i = 0; i < 100; ++i) {
            const auto k = static_cast<event_kind>(kind(rng));
            events.push_back({ ts, { .player = player(rng), .kind = k, .score = score(rng), .session_ms = ts * 10 } });
        }
    }

    auto run = [&](std::size_t leaves, std::size_t fan_in) {
        std::map<fold::window_id, analytics::report> out;
        fold::tree<analytics> t(cfg, [&](auto w, const auto& r) { out.insert_or_assign(w, r); });
        const auto shape = fold::build(t, leaves, [&](std::size_t) { return fan_in; });
        std::mt19937_64 route(leaves);
        std::uniform_int_distribution<std::size_t> any(0, leaves - 1);

        fold::timestamp last = 0;
        for (const auto& [ts, e] : events) {
            if (ts / 10 != last / 10) {
                for (auto l : shape.leaves) { t.at(l).advance(ts); }
                t.tick();
            }
            last = ts;
            t.at(shape.leaves[any(route)]).ingest(ts, e);
        }
        for (auto l : shape.leaves) { t.at(l).advance(1'000); }
        t.tick();
        return out;
    };

    const auto flat = run(1, 2);
    for (auto [leaves, fan_in] : { std::pair { 7UZ, 2UZ }, { 50UZ, 3UZ }, { 400UZ, 20UZ } }) {
        const auto hier = run(leaves, fan_in);
        ASSERT_EQ(hier.size(), flat.size());
        for (const auto& [w, f] : flat) {
            const auto& h = hier.at(w);
            EXPECT_EQ(h.event_counts, f.event_counts);
            EXPECT_EQ(h.active_players, f.active_players);
            EXPECT_EQ(h.top, f.top);
            EXPECT_EQ(h.avg_session_s, f.avg_session_s);
            EXPECT_NEAR(h.p50_score, f.p50_score, 20.0);
            EXPECT_NEAR(h.p99_score, f.p99_score, 20.0);
        }
    }
}
