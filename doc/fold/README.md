# fold — hierarchical aggregation of mergeable partials

Experimental. Header-only, `libfold/`.

Streaming analytics over a tree of nodes whose depth and fan-in are decided
by the deployment, not by the processing code. Raw events never leave the
edge; only bounded-size partials travel upward.

## The contract

Processing code is a plain type with three pure functions:

```cpp
struct player_analytics {
    using event = player_event;
    struct partial { /* counters, sketches */ };

    static auto lift(const event&) -> partial;
    static auto merge(partial, const partial&) -> partial;   // associative, commutative
    static auto lower(const partial&) -> report;
};
static_assert(fold::aggregation<player_analytics>);
```

A default-constructed `partial` is the identity of `merge`. That is the whole
API surface for the author: no node ids, layers, windows, watermarks or
transport. See `libfold/player_analytics.hpp`.

Because `merge` is closed over `partial`, a node cannot tell whether its
inputs came from events or from other nodes, so the same code runs at every
layer. Keep `partial` bounded (counters, sketches) and a node's cost depends
on its fan-in only.

## Runtime

- **`node`**: ingests events (edge), receives child envelopes (inner),
  finalizes windows (root); any mix of those.
- **`tree`** stands in for the deployment layer: builds shapes, re-parents
  subtrees, moves envelopes. A real deployment swaps it for per-node timers
  and a transport.

Envelope semantics, which make delivery and re-parenting safe for any
`merge`, idempotent or not:

| Concern            | Mechanism                                                       |
|--------------------|-----------------------------------------------------------------|
| Duplicates/retries | Envelopes carry the *cumulative* value per changed window; the parent replaces per (child, window) and ignores epochs it has seen |
| Event time         | Tumbling windows; watermark = min(local source, active children), monotonic |
| Re-parenting       | Old parent keeps the child's windows that are final by its last watermark and drops the rest; the child re-sends all open windows to the new parent |
| Emptied nodes      | A node with no source and no active child reports itself idle and is skipped in the minimum |
| Late data          | Counted, not silently merged (`late_events`, `late_partials`) |

## Sketches (`libfold/sketch.hpp`)

| Type          | Merge                     | Topology-independent      |
|---------------|---------------------------|---------------------------|
| `hyperloglog` | register max (sparse → dense) | exactly                |
| `top_k`       | best value per id, total order | exactly              |
| `tdigest`     | centroid merge            | within the error bound    |

## Running

```bash
baldr -p . -b Release -t example-fold --run
baldr -p . -b Debug -t test-fold --run
```

`example-fold` puts 50k players behind edges holding 2–8 players each, builds
a tree with random fan-in 2–20 (5 layers), re-parents a subtree mid-window and
lets some players reconnect through other edges. It then compares against a
single node fed the same events: counts, distinct players, leaderboard and
session averages are identical; quantiles agree to ~0.1%.

Options: `--players N`, `--leaf-max N`, `--fan-in-max N`, `--seed N`.

## Not covered yet

- Serialization / transport (partials are in-memory values).
- Real per-node flush timers; `tree::tick` propagates root-ward in one round,
  so it hides the depth × flush-interval latency of a real deployment.
- At-least-once delivery with acks; current delivery is synchronous.
- A child moved under a parent whose watermark is ahead of it loses the
  windows that parent already closed (counted as `late_partials`).
- Cross-key joins; co-partition at ingest instead.
