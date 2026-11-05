/**
 * Part of Nexus Library
 *
 * One fold node. The same type serves as edge (events enter via `ingest`),
 * intermediate (partials enter via `receive`) or root (`flush(true)` hands
 * back finalized windows); a node can be any mix of those at once.
 *
 * Wire semantics
 * --------------
 * An envelope carries, per changed window, the node's *cumulative* partial for
 * that window (not a delta). A parent stores the latest value per (child,
 * window) and replaces it on update. Combined with a per-child epoch this
 * makes delivery idempotent: a re-sent or duplicated envelope cannot double
 * count, which is what makes re-parenting and retries safe for any `merge`,
 * idempotent or not.
 *
 * Time
 * ----
 * Windows are tumbling, by event time. A node's watermark is the minimum over
 * its local source (`advance`) and its attached children, and never moves
 * backwards. A window closes once `end + allowed_lateness <= watermark`.
 * A node only closes a window after sending its final value, so a parent can
 * tell from a child's watermark which of the child's windows are final.
 * A node with neither a source nor an active child (e.g. emptied by a
 * re-parent) reports itself idle, and parents leave it out of the minimum
 * instead of stalling on it.
 *
 * @author      Gábor Krisztián Girhiny
 * @coauthor    Claude Opus 5.5 (Claude)
 * @date        2026-11-05
 */

#pragma once

#include "libfold/aggregation.hpp"

#include <algorithm>
#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <utility>
#include <vector>

namespace fold {

using node_id   = std::uint32_t;
using window_id = std::int64_t;

struct window_config {
    timestamp width            = 60'000;
    timestamp allowed_lateness = 0;

    [[nodiscard]] constexpr auto window_of(timestamp ts) const -> window_id {
        return ts >= 0 ? ts / width : -((-ts + width - 1) / width);
    }

    [[nodiscard]] constexpr auto start_of(window_id w) const -> timestamp { return w * width; }
    [[nodiscard]] constexpr auto end_of(window_id w) const -> timestamp { return (w + 1) * width; }
    [[nodiscard]] constexpr auto closes_at(window_id w) const -> timestamp { return end_of(w) + allowed_lateness; }
};

template <aggregation A>
struct envelope {
    node_id source;
    std::uint64_t epoch;                                            ///< strictly increasing per source
    std::optional<timestamp> watermark;                             ///< nullopt: no progress yet
    bool idle = false;                                              ///< no source, no active child: parents skip it
    std::vector<std::pair<window_id, typename A::partial>> windows; ///< cumulative value of changed windows
};

struct node_stats {
    std::uint64_t events              = 0;
    std::uint64_t late_events         = 0;  ///< events for a window this node already closed
    std::uint64_t late_partials       = 0;  ///< child partials for a window this node already closed
    std::uint64_t duplicate_envelopes = 0;  ///< epoch not newer than the last seen one
    std::uint64_t rejected_envelopes  = 0;  ///< from a node that is not an attached child
    std::uint64_t flushes             = 0;
    std::uint64_t merges              = 0;
};

template <aggregation A>
class node {
public:
    using event   = typename A::event;
    using partial = typename A::partial;

    struct flush_result {
        envelope<A> out;
        std::vector<std::pair<window_id, partial>> finalized;   ///< filled only when asked to finalize
    };

    node(node_id id, window_config cfg)
        : m_id(id)
        , m_cfg(cfg)
    {}

    [[nodiscard]] auto id() const -> node_id { return m_id; }
    [[nodiscard]] auto watermark() const -> std::optional<timestamp> { return m_wm; }
    [[nodiscard]] auto stats() const -> const node_stats& { return m_stats; }

    // ---- Source side --------------------------------------------------------

    /// @return false if the event belongs to a window this node already closed.
    auto ingest(timestamp ts, const event& evt) -> bool {
        const auto w = m_cfg.window_of(ts);
        if (is_closed(w)) {
            ++m_stats.late_events;
            return false;
        }

        auto& acc = m_local[w];
        acc = A::merge(std::move(acc), A::lift(evt));
        ++m_stats.events;
        ++m_stats.merges;
        m_dirty.insert(w);
        return true;
    }

    /// Source promise: no further events with a timestamp below `wm`.
    void advance(timestamp wm) {
        m_source_wm = m_source_wm ? std::max(*m_source_wm, wm) : wm;
    }

    // ---- Topology side (driven by the deployment layer) ---------------------

    void attach(node_id child) {
        m_children[child].attached = true;
    }

    /**
     * Stop accepting `child`. Its windows that are final (by its last
     * watermark) stay, everything else is dropped: the child will report
     * those to its new parent in full, so keeping them would double count.
     */
    void detach(node_id child) {
        auto it = m_children.find(child);
        if (it == m_children.end()) {
            return;
        }

        auto& state = it->second;
        state.attached = false;
        std::erase_if(state.windows, [&](const auto& kv) {
            const bool is_final = state.watermark && m_cfg.closes_at(kv.first) <= *state.watermark;
            if (not is_final) {
                m_dirty.insert(kv.first);   // our cumulative value shrank, tell the parent
            }
            return not is_final;
        });

        if (state.windows.empty()) {
            m_children.erase(it);
        }
    }

    /// After moving to a new parent: the next flush must carry every open window.
    void mark_all_dirty() {
        for (const auto& [w, _] : m_local) { m_dirty.insert(w); }
        for (const auto& [_, state] : m_children) {
            for (const auto& [w, __] : state.windows) { m_dirty.insert(w); }
        }
    }

    auto receive(const envelope<A>& env) -> bool {
        auto it = m_children.find(env.source);
        if (it == m_children.end() || not it->second.attached) {
            ++m_stats.rejected_envelopes;
            return false;
        }

        auto& state = it->second;
        if (env.epoch <= state.epoch) {
            ++m_stats.duplicate_envelopes;
            return false;
        }

        state.epoch = env.epoch;
        state.idle  = env.idle;
        if (env.watermark) {
            state.watermark = state.watermark ? std::max(*state.watermark, *env.watermark) : *env.watermark;
        }

        for (const auto& [w, value] : env.windows) {
            if (is_closed(w)) {
                ++m_stats.late_partials;
                continue;
            }
            state.windows.insert_or_assign(w, value);
            m_dirty.insert(w);
        }
        return true;
    }

    /**
     * Emit changed windows upward and close the windows the watermark has
     * passed. Closed windows are returned in `finalized` when `finalize` is
     * set (the root asks for that; inner nodes don't need the extra merge).
     */
    auto flush(bool finalize) -> flush_result {
        update_watermark();

        flush_result result {
            .out = envelope<A> {
                .source    = m_id,
                .epoch     = ++m_epoch,
                .watermark = m_idle ? std::nullopt : m_wm,
                .idle      = m_idle,
                .windows   = {},
            },
            .finalized = {},
        };

        result.out.windows.reserve(m_dirty.size());
        for (auto w : m_dirty) {
            result.out.windows.emplace_back(w, merged(w));
        }
        m_dirty.clear();

        for (auto w : closing_windows()) {
            if (finalize) {
                result.finalized.emplace_back(w, merged(w));
            }
            m_local.erase(w);
            for (auto& [_, state] : m_children) {
                state.windows.erase(w);
            }
        }

        std::erase_if(m_children, [](const auto& kv) {
            return not kv.second.attached && kv.second.windows.empty();
        });

        ++m_stats.flushes;
        return result;
    }

    [[nodiscard]] auto open_windows() const -> std::size_t {
        return all_windows().size();
    }

private:
    struct child_state {
        std::uint64_t epoch = 0;
        std::optional<timestamp> watermark;
        std::map<window_id, partial> windows;
        bool attached = false;
        bool idle     = false;
    };

    node_id m_id;
    window_config m_cfg;

    std::optional<timestamp> m_source_wm;
    std::optional<timestamp> m_wm;
    bool m_idle = false;
    std::uint64_t m_epoch = 0;

    std::map<window_id, partial> m_local;
    std::map<node_id, child_state> m_children;
    std::set<window_id> m_dirty;

    node_stats m_stats;

    [[nodiscard]] auto is_closed(window_id w) const -> bool {
        return m_wm && m_cfg.closes_at(w) <= *m_wm;
    }

    void update_watermark() {
        std::optional<timestamp> low = m_source_wm;
        bool has_active = false;
        bool holding    = false;

        for (const auto& [_, state] : m_children) {
            if (not state.attached || state.idle) {
                continue;
            }
            has_active = true;
            if (not state.watermark) {
                holding = true;     // an attached child hasn't reported yet
                continue;
            }
            low = low ? std::min(*low, *state.watermark) : *state.watermark;
        }

        m_idle = not m_source_wm && not has_active;
        if (holding) {
            return;
        }

        if (low) {
            m_wm = m_wm ? std::max(*m_wm, *low) : *low;
        }
    }

    [[nodiscard]] auto merged(window_id w) -> partial {
        partial acc {};
        if (auto it = m_local.find(w); it != m_local.end()) {
            acc = A::merge(std::move(acc), it->second);
            ++m_stats.merges;
        }
        for (const auto& [_, state] : m_children) {
            if (auto it = state.windows.find(w); it != state.windows.end()) {
                acc = A::merge(std::move(acc), it->second);
                ++m_stats.merges;
            }
        }
        return acc;
    }

    [[nodiscard]] auto all_windows() const -> std::set<window_id> {
        std::set<window_id> ws;
        for (const auto& [w, _] : m_local) { ws.insert(w); }
        for (const auto& [_, state] : m_children) {
            for (const auto& [w, __] : state.windows) { ws.insert(w); }
        }
        return ws;
    }

    [[nodiscard]] auto closing_windows() const -> std::vector<window_id> {
        std::vector<window_id> ws;
        for (auto w : all_windows()) {
            if (is_closed(w)) {
                ws.push_back(w);
            }
        }
        return ws;
    }
};

} // namespace fold
