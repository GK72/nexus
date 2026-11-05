#pragma once

#include <libfold/aggregation.hpp>

#include <compare>
#include <cstdint>

/// Exact, non-idempotent aggregation: any double count shows up immediately.
struct sum_count {
    using event = std::int64_t;

    struct partial {
        std::int64_t sum   = 0;
        std::int64_t count = 0;
        auto operator<=>(const partial&) const = default;
    };

    static auto lift(const event& e) -> partial { return { .sum = e, .count = 1 }; }

    static auto merge(partial a, const partial& b) -> partial {
        a.sum   += b.sum;
        a.count += b.count;
        return a;
    }

    static auto lower(const partial& p) -> partial { return p; }
};

static_assert(fold::aggregation<sum_count>);

struct no_identity {
    using event = int;
    struct partial {
        explicit partial(int v) : value(v) {}
        int value;
    };
    static auto lift(const event& e) -> partial { return partial { e }; }
    static auto merge(partial a, const partial&) -> partial { return a; }
    static auto lower(const partial& p) -> int { return p.value; }
};

static_assert(not fold::aggregation<no_identity>, "a partial without an identity is rejected");
