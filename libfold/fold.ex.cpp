/**
 * Part of Nexus Library
 *
 * Example: tens of thousands of players behind edge nodes that can each
 * handle only a handful of them, aggregated through a tree of random depth
 * and fan-in. A subtree is re-parented mid-window and some players reconnect
 * to a different edge. The result is compared against a single flat node
 * that saw the very same events.
 *
 * @author      Gábor Krisztián Girhiny
 * @coauthor    Claude Opus 5.5 (Claude)
 * @date        2026-11-05
 */

#include "libfold/player_analytics.hpp"
#include "libfold/tree.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <format>
#include <iostream>
#include <map>
#include <random>
#include <string_view>
#include <vector>

namespace {

using analytics = game::player_analytics;
using report    = analytics::report;

struct options {
    std::size_t players       = 50'000;
    std::size_t leaf_min      = 2;      ///< players per edge node
    std::size_t leaf_max      = 8;
    std::size_t fan_in_min    = 2;      ///< children per inner node
    std::size_t fan_in_max    = 20;
    fold::timestamp duration  = 180'000;
    fold::timestamp tick      = 5'000;
    std::uint64_t seed        = 72;
};

struct routed_event {
    fold::timestamp ts;
    std::size_t leaf;   ///< index into layout.leaves
    game::player_event event;
};

auto parse(int argc, char** argv) -> options {
    options opts;
    for (int i = 1; i + 1 < argc; i += 2) {
        const std::string_view key = argv[i];
        const auto value = std::strtoull(argv[i + 1], nullptr, 10);
        if (key == "--players")    { opts.players    = value; }
        if (key == "--leaf-max")   { opts.leaf_max   = std::max<std::size_t>(opts.leaf_min, value); }
        if (key == "--fan-in-max") { opts.fan_in_max = std::max<std::size_t>(opts.fan_in_min, value); }
        if (key == "--seed")       { opts.seed       = value; }
    }
    return opts;
}

/// Assign players to edge nodes of random capacity; returns the edge count.
auto assign_players(const options& opts, std::mt19937_64& rng, std::vector<std::size_t>& home) -> std::size_t {
    std::uniform_int_distribution<std::size_t> capacity(opts.leaf_min, opts.leaf_max);
    std::size_t leaf = 0;
    std::size_t left = capacity(rng);
    for (std::size_t p = 0; p < opts.players; ++p) {
        if (left == 0) {
            ++leaf;
            left = capacity(rng);
        }
        home[p] = leaf;
        --left;
    }
    return leaf + 1;
}

auto generate(const options& opts, std::mt19937_64& rng, std::vector<std::size_t> home, std::size_t leaves)
    -> std::vector<routed_event>
{
    std::vector<routed_event> events;
    std::uniform_int_distribution<fold::timestamp> start(0, opts.duration - 20'000);
    std::uniform_int_distribution<fold::timestamp> match_gap(8'000, 40'000);
    std::uniform_int_distribution<fold::timestamp> session(30'000, 150'000);
    std::uniform_real_distribution<double> skill(500.0, 1500.0);
    std::normal_distribution<double> noise(0.0, 150.0);
    std::uniform_int_distribution<std::size_t> any_leaf(0, leaves - 1);
    std::bernoulli_distribution reconnect(0.05);
    std::bernoulli_distribution purchase(0.10);

    for (std::size_t p = 0; p < opts.players; ++p) {
        const auto id    = static_cast<std::uint64_t>(p) + 1;
        const auto level = skill(rng);
        const auto login = start(rng);
        const auto until = std::min(login + session(rng), opts.duration - 1);
        auto leaf = home[p];
        auto t    = login;

        events.push_back({ t, leaf, { .player = id, .kind = game::event_kind::login } });
        for (t += match_gap(rng); t < until; t += match_gap(rng)) {
            const auto score = static_cast<std::uint32_t>(std::max(0.0, level + noise(rng)));
            events.push_back({ t, leaf, { .player = id, .kind = game::event_kind::match_end, .score = score } });
            if (purchase(rng)) {
                events.push_back({ t + 1, leaf, { .player = id, .kind = game::event_kind::purchase } });
            }
            if (reconnect(rng)) {
                leaf = any_leaf(rng);   // dropped and came back through another edge
            }
        }
        const auto logout = until;
        events.push_back({ logout, leaf, {
            .player = id, .kind = game::event_kind::logout, .session_ms = logout - login } });
    }

    std::ranges::stable_sort(events, {}, &routed_event::ts);
    return events;
}

auto same_top(const report& lhs, const report& rhs) -> bool {
    return lhs.top == rhs.top;
}

auto rel(double lhs, double rhs) -> double {
    return rhs == 0.0 ? 0.0 : std::abs(lhs - rhs) / rhs * 100.0;
}

} // namespace

auto main(int argc, char** argv) -> int {
    const auto opts = parse(argc, argv);
    std::mt19937_64 rng(opts.seed);

    const fold::window_config windows { .width = 60'000, .allowed_lateness = 5'000 };

    // ---- Topology ----------------------------------------------------------

    std::vector<std::size_t> home(opts.players);
    const auto leaf_count = assign_players(opts, rng, home);

    std::map<fold::window_id, report> hier_results;
    std::map<fold::window_id, report> flat_results;

    fold::tree<analytics> hier(windows, [&](auto w, const report& r) { hier_results.insert_or_assign(w, r); });
    fold::tree<analytics> flat(windows, [&](auto w, const report& r) { flat_results.insert_or_assign(w, r); });

    std::uniform_int_distribution<std::size_t> fan_in(opts.fan_in_min, opts.fan_in_max);
    const auto shape = fold::build(hier, leaf_count, [&](std::size_t) { return fan_in(rng); });
    const auto single = flat.add_node();

    const auto events = generate(opts, rng, home, leaf_count);

    std::cout << std::format(
        "players {}  edges {} ({}-{} players each)  nodes {}  layers {}  fan-in {}-{}  events {}\n\n",
        opts.players, leaf_count, opts.leaf_min, opts.leaf_max,
        hier.size(), hier.depth(), opts.fan_in_min, opts.fan_in_max, events.size()
    );

    // Pick a subtree to re-parent mid-run: a grandchild of the root moves to
    // a different child of the root.
    std::optional<std::pair<fold::node_id, fold::node_id>> move_plan;
    if (const auto& top = hier.children_of(shape.root); top.size() >= 2) {
        if (const auto& below = hier.children_of(top[0]); not below.empty()) {
            move_plan = std::pair { below.front(), top[1] };
        }
    }

    // ---- Run ---------------------------------------------------------------

    const auto started = std::chrono::steady_clock::now();
    std::size_t next = 0;

    for (fold::timestamp now = opts.tick; now <= opts.duration + windows.allowed_lateness + opts.tick; now += opts.tick) {
        for (; next < events.size() && events[next].ts < now; ++next) {
            const auto& e = events[next];
            hier.at(shape.leaves[e.leaf]).ingest(e.ts, e.event);
            flat.at(single).ingest(e.ts, e.event);
        }

        const auto watermark = std::min(now, opts.duration);
        for (auto leaf : shape.leaves) {
            hier.at(leaf).advance(watermark);
        }
        flat.at(single).advance(watermark);

        if (move_plan && now == opts.duration / 2 + 5'000) {
            const auto [subtree, target] = *move_plan;
            std::cout << std::format("t={}s  moving subtree {} under node {} (mid-window)\n\n", now / 1000, subtree, target);
            hier.move(subtree, target);
        }

        hier.tick();
        flat.tick();
    }

    // Flush whatever is left.
    for (auto leaf : shape.leaves) { hier.at(leaf).advance(opts.duration + windows.width * 2); }
    flat.at(single).advance(opts.duration + windows.width * 2);
    hier.tick();
    flat.tick();

    const auto elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();

    // ---- Report ------------------------------------------------------------

    std::cout << std::format("{:>6} | {:>22} | {:>24} | {:>24} | {:>12} | {:>8} | {}\n",
        "window", "active players", "p50 score", "p99 score", "avg session", "events", "top-10");
    std::cout << std::string(130, '-') << '\n';

    bool exact_ok = true;
    for (const auto& [w, flat_r] : flat_results) {
        auto it = hier_results.find(w);
        if (it == hier_results.end()) {
            std::cout << std::format("{:>6} | missing in tree result\n", w);
            exact_ok = false;
            continue;
        }
        const auto& hier_r = it->second;

        const bool counts = hier_r.event_counts == flat_r.event_counts;
        const bool active = hier_r.active_players == flat_r.active_players;
        const bool top    = same_top(hier_r, flat_r);
        const bool avg    = hier_r.avg_session_s == flat_r.avg_session_s;
        exact_ok = exact_ok && counts && active && top && avg;

        std::uint64_t total = 0;
        for (auto c : hier_r.event_counts) { total += c; }

        std::cout << std::format(
            "{:>6} | {:>10.0f} {:>11} | {:>7.1f} vs {:>7.1f} {:>5.2f}% | {:>7.1f} vs {:>7.1f} {:>5.2f}% | {:>9.1f}s {} | {:>7} {} | {}\n",
            w,
            hier_r.active_players, active ? "same" : "DIFF",
            hier_r.p50_score, flat_r.p50_score, rel(hier_r.p50_score, flat_r.p50_score),
            hier_r.p99_score, flat_r.p99_score, rel(hier_r.p99_score, flat_r.p99_score),
            hier_r.avg_session_s, avg ? "=" : "!",
            total, counts ? "=" : "!",
            top ? "same" : "DIFF"
        );
    }

    const auto stats = hier.total_stats();
    std::cout << std::format(
        "\nexact parts (counts, distinct players, leaderboard, sessions) identical to flat: {}\n"
        "tree: {} merges, {} flushes, {} late events, {} late partials, {} duplicate envelopes\n"
        "wall time {:.2f}s (tree + flat baseline)\n",
        exact_ok ? "yes" : "NO",
        stats.merges, stats.flushes, stats.late_events, stats.late_partials, stats.duplicate_envelopes,
        elapsed
    );

    // Partial size does not grow with the number of players behind it.
    analytics::partial few;
    analytics::partial all;
    for (const auto& e : events) {
        if (e.leaf == 0) { few = analytics::merge(std::move(few), analytics::lift(e.event)); }
        all = analytics::merge(std::move(all), analytics::lift(e.event));
    }
    std::cout << std::format(
        "partial size: one edge {} B, all {} players {} B (bounded by sketch sizes)\n",
        few.bytes(), opts.players, all.bytes()
    );

    return exact_ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
