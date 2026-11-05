/**
 * Part of Nexus Library
 *
 * In-process stand-in for the deployment layer: owns nodes, wires them into
 * a forest of any shape, re-parents subtrees, and moves envelopes upward.
 *
 * A real deployment replaces `tick` with independent per-node flush timers
 * and a transport; nothing in `node` or the user's aggregation changes.
 *
 * @author      Gábor Krisztián Girhiny
 * @coauthor    Claude Opus 5.5 (Claude)
 * @date        2026-11-05
 */

#pragma once

#include "libfold/aggregation.hpp"
#include "libfold/node.hpp"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace fold {

template <aggregation A>
class tree {
public:
    using sink = std::function<void(window_id, const result_t<A>&)>;

    tree(window_config cfg, sink on_result)
        : m_cfg(cfg)
        , m_sink(std::move(on_result))
    {}

    auto add_node() -> node_id {
        const auto id = static_cast<node_id>(m_nodes.size());
        m_nodes.push_back(std::make_unique<node<A>>(id, m_cfg));
        m_parent.emplace_back();
        m_children.emplace_back();
        return id;
    }

    void attach(node_id child, node_id parent) {
        if (m_parent.at(child)) {
            throw std::logic_error("fold::tree: node already has a parent, use move()");
        }
        if (in_subtree(parent, child)) {
            throw std::logic_error("fold::tree: attach would create a cycle");
        }
        m_parent[child] = parent;
        m_children.at(parent).push_back(child);
        m_nodes[parent]->attach(child);
    }

    /// Re-parent `child` (and everything below it) under `new_parent`.
    void move(node_id child, node_id new_parent) {
        if (in_subtree(new_parent, child)) {
            throw std::logic_error("fold::tree: move would create a cycle");
        }
        if (auto old = m_parent.at(child)) {
            m_nodes[*old]->detach(child);
            std::erase(m_children[*old], child);
            m_parent[child].reset();
        }
        attach(child, new_parent);
        m_nodes[child]->mark_all_dirty();
    }

    [[nodiscard]] auto at(node_id id) -> node<A>& { return *m_nodes.at(id); }
    [[nodiscard]] auto at(node_id id) const -> const node<A>& { return *m_nodes.at(id); }
    [[nodiscard]] auto parent_of(node_id id) const -> std::optional<node_id> { return m_parent.at(id); }
    [[nodiscard]] auto children_of(node_id id) const -> const std::vector<node_id>& { return m_children.at(id); }
    [[nodiscard]] auto size() const -> std::size_t { return m_nodes.size(); }

    [[nodiscard]] auto roots() const -> std::vector<node_id> {
        std::vector<node_id> result;
        for (node_id id = 0; id < m_nodes.size(); ++id) {
            if (not m_parent[id]) {
                result.push_back(id);
            }
        }
        return result;
    }

    /// Number of layers on the longest root-to-leaf path.
    [[nodiscard]] auto depth() const -> std::size_t {
        std::size_t deepest = 0;
        for (node_id id = 0; id < m_nodes.size(); ++id) {
            std::size_t d = 1;
            for (auto p = m_parent[id]; p; p = m_parent[*p]) {
                ++d;
            }
            deepest = std::max(deepest, d);
        }
        return deepest;
    }

    /**
     * One round: flush every node children-first and deliver each envelope
     * to its parent, so a change travels root-ward within the round. Roots
     * lower their finalized windows into the sink.
     */
    void tick() {
        for (auto id : post_order()) {
            const auto parent = m_parent[id];
            auto flushed = m_nodes[id]->flush(not parent.has_value());

            if (parent) {
                m_nodes[*parent]->receive(flushed.out);
            }
            else {
                for (const auto& [w, value] : flushed.finalized) {
                    m_sink(w, A::lower(value));
                }
            }
        }
    }

    [[nodiscard]] auto total_stats() const -> node_stats {
        node_stats sum;
        for (const auto& n : m_nodes) {
            const auto& s = n->stats();
            sum.events              += s.events;
            sum.late_events         += s.late_events;
            sum.late_partials       += s.late_partials;
            sum.duplicate_envelopes += s.duplicate_envelopes;
            sum.rejected_envelopes  += s.rejected_envelopes;
            sum.flushes             += s.flushes;
            sum.merges              += s.merges;
        }
        return sum;
    }

private:
    window_config m_cfg;
    sink m_sink;

    std::vector<std::unique_ptr<node<A>>> m_nodes;
    std::vector<std::optional<node_id>> m_parent;
    std::vector<std::vector<node_id>> m_children;

    /// Is `id` inside the subtree rooted at `root` (inclusive)?
    [[nodiscard]] auto in_subtree(node_id id, node_id root) const -> bool {
        for (std::optional<node_id> cur = id; cur; cur = m_parent.at(*cur)) {
            if (*cur == root) {
                return true;
            }
        }
        return false;
    }

    [[nodiscard]] auto post_order() const -> std::vector<node_id> {
        std::vector<node_id> order;
        order.reserve(m_nodes.size());

        std::vector<std::pair<node_id, std::size_t>> stack;
        for (auto root : roots()) {
            stack.emplace_back(root, 0);
            while (not stack.empty()) {
                auto& [id, next] = stack.back();
                if (next < m_children[id].size()) {
                    const auto child = m_children[id][next++];
                    stack.emplace_back(child, 0);
                }
                else {
                    order.push_back(id);
                    stack.pop_back();
                }
            }
        }
        return order;
    }
};

struct layout {
    std::vector<node_id> leaves;
    node_id root;
    std::size_t layers;
};

/**
 * @brief   Build a tree over `leaf_count` edge nodes, grouping bottom-up.
 *
 * `fan_in(layer)` is asked once per new parent, so capacities may vary node
 * by node (weak and strong hardware mixed). Values below 2 are raised to 2.
 */
template <aggregation A>
auto build(tree<A>& t, std::size_t leaf_count, const std::function<std::size_t(std::size_t)>& fan_in) -> layout {
    if (leaf_count == 0) {
        throw std::invalid_argument("fold::build: need at least one leaf");
    }

    layout result { .leaves = {}, .root = 0, .layers = 1 };
    for (std::size_t i = 0; i < leaf_count; ++i) {
        result.leaves.push_back(t.add_node());
    }

    auto current = result.leaves;
    while (current.size() > 1) {
        std::vector<node_id> next;
        for (std::size_t i = 0; i < current.size();) {
            const auto cap    = std::max<std::size_t>(2, fan_in(result.layers));
            const auto parent = t.add_node();
            for (std::size_t j = 0; j < cap && i < current.size(); ++j, ++i) {
                t.attach(current[i], parent);
            }
            next.push_back(parent);
        }
        current = std::move(next);
        ++result.layers;
    }

    result.root = current.front();
    return result;
}

} // namespace fold
