/**
 * Part of Nexus Library
 *
 * Example processing code: online-game player analytics.
 *
 * This is *everything* the analytics author writes. No node ids, layers,
 * transport, windows or watermarks appear here; the runtime attaches event
 * time and decides where `merge` runs.
 *
 * @author      Gábor Krisztián Girhiny
 * @coauthor    Claude Opus 5.5 (Claude)
 * @date        2026-11-05
 */

#pragma once

#include "libfold/sketch.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace game {

enum class event_kind : std::uint8_t { login, match_end, purchase, logout };
inline constexpr std::size_t event_kinds = 4;

struct player_event {
    std::uint64_t player;
    event_kind kind;
    std::uint32_t score      = 0;   ///< match_end
    std::int64_t  session_ms = 0;   ///< logout
};

struct player_analytics {
    using event = player_event;

    struct partial {
        fold::hyperloglog active_players { 14 };
        fold::tdigest scores { 100 };
        fold::top_k<std::uint64_t, std::uint32_t> leaderboard { 10 };
        std::array<std::uint64_t, event_kinds> event_counts {};
        std::uint64_t sessions   = 0;
        std::int64_t  session_ms = 0;

        [[nodiscard]] auto bytes() const -> std::size_t {
            return active_players.bytes() + scores.bytes() + leaderboard.bytes()
                 + sizeof(event_counts) + sizeof(sessions) + sizeof(session_ms);
        }
    };

    struct report {
        double active_players;
        double p50_score;
        double p99_score;
        std::vector<fold::top_k<std::uint64_t, std::uint32_t>::entry> top;
        double avg_session_s;
        std::array<std::uint64_t, event_kinds> event_counts;
    };

    static auto lift(const event& e) -> partial {
        partial p;
        p.active_players.add(e.player);
        p.event_counts[static_cast<std::size_t>(e.kind)] = 1;

        if (e.kind == event_kind::match_end) {
            p.scores.add(e.score);
            p.leaderboard.offer(e.player, e.score);
        }
        if (e.kind == event_kind::logout) {
            p.sessions   = 1;
            p.session_ms = e.session_ms;
        }
        return p;
    }

    static auto merge(partial a, const partial& b) -> partial {
        a.active_players.merge(b.active_players);
        a.scores.merge(b.scores);
        a.leaderboard.merge(b.leaderboard);
        for (std::size_t i = 0; i < event_kinds; ++i) {
            a.event_counts[i] += b.event_counts[i];
        }
        a.sessions   += b.sessions;
        a.session_ms += b.session_ms;
        return a;
    }

    static auto lower(const partial& p) -> report {
        return {
            .active_players = p.active_players.estimate(),
            .p50_score      = p.scores.quantile(0.50),
            .p99_score      = p.scores.quantile(0.99),
            .top            = p.leaderboard.items(),
            .avg_session_s  = p.sessions != 0
                ? static_cast<double>(p.session_ms) / static_cast<double>(p.sessions) / 1000.0
                : 0.0,
            .event_counts   = p.event_counts,
        };
    }
};

} // namespace game
