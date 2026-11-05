/**
 * Part of Nexus Library
 *
 * The contract between processing code and the fold runtime.
 *
 * Processing code is a plain type with three pure functions:
 *
 *   lift  : event              -> partial
 *   merge : partial, partial   -> partial   (associative, commutative)
 *   lower : partial            -> result
 *
 * A default-constructed `partial` must be the identity of `merge`.
 *
 * Because `merge` is closed over `partial`, a node cannot tell whether its
 * inputs came from raw events or from other nodes. The same code therefore
 * runs at every layer of a tree whose depth and fan-in are unknown to it.
 * Keep `partial` bounded in size (counters, sketches) so the cost of a node
 * depends on its fan-in, not on how many sources are below it.
 *
 * @author      Gábor Krisztián Girhiny
 * @coauthor    Claude Opus 5.5 (Claude)
 * @date        2026-11-05
 */

#pragma once

#include <concepts>
#include <cstdint>
#include <utility>

namespace fold {

/// Event time in milliseconds. Attached by the runtime, never by user code.
using timestamp = std::int64_t;

template <typename A>
concept aggregation = requires(
    const typename A::event& evt,
    typename A::partial lhs,
    const typename A::partial& rhs
) {
    requires std::default_initializable<typename A::partial>;
    requires std::copyable<typename A::partial>;

    { A::lift(evt) }               -> std::same_as<typename A::partial>;
    { A::merge(std::move(lhs), rhs) } -> std::same_as<typename A::partial>;
    { A::lower(rhs) };
};

template <aggregation A>
using result_t = decltype(A::lower(std::declval<const typename A::partial&>()));

} // namespace fold
