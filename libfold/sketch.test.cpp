#include <libfold/sketch.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <map>
#include <random>
#include <vector>

TEST(HyperLogLog, EstimateWithinErrorBound) {
    // p = 14: standard error ~0.81%; allow 4%.
    for (std::uint64_t n : { 10U, 100U, 1'000U, 10'000U, 50'000U, 200'000U }) {
        fold::hyperloglog hll(14);
        for (std::uint64_t i = 0; i < n; ++i) {
            hll.add(i);
            hll.add(i);     // duplicates must not count
        }
        const auto err = std::abs(hll.estimate() - static_cast<double>(n)) / static_cast<double>(n);
        EXPECT_LT(err, 0.04) << "n = " << n;
    }
}

TEST(HyperLogLog, EmptyIsZero) {
    EXPECT_EQ(fold::hyperloglog {}.estimate(), 0.0);
}

TEST(HyperLogLog, SparseSwitchesToDense) {
    fold::hyperloglog hll(10);
    EXPECT_FALSE(hll.is_dense());
    for (std::uint64_t i = 0; i < 5'000; ++i) {
        hll.add(i);
    }
    EXPECT_TRUE(hll.is_dense());
    EXPECT_EQ(hll.bytes(), 1024U);
}

TEST(HyperLogLog, MergeEqualsAddingEverything) {
    fold::hyperloglog all;
    fold::hyperloglog lhs;
    fold::hyperloglog rhs;
    for (std::uint64_t i = 0; i < 30'000; ++i) {
        all.add(i);
        (i % 3 == 0 ? lhs : rhs).add(i);
    }

    auto lr = lhs; lr.merge(rhs);
    auto rl = rhs; rl.merge(lhs);
    EXPECT_EQ(lr, all);
    EXPECT_EQ(rl, all);

    auto twice = lr;
    twice.merge(rhs);   // idempotent
    EXPECT_EQ(twice, all);
}

TEST(HyperLogLog, SparseAndDenseCompareByValue) {
    fold::hyperloglog sparse;
    fold::hyperloglog dense;
    for (std::uint64_t i = 0; i < 10; ++i) {
        sparse.add(i);
    }
    fold::hyperloglog big;
    for (std::uint64_t i = 0; i < 100'000; ++i) {
        big.add(i);
    }
    dense = big;
    dense.merge(sparse);
    big.merge(sparse);
    EXPECT_EQ(dense, big);
    EXPECT_FALSE(sparse == big);
}

TEST(TDigest, QuantilesOfUniform) {
    std::mt19937_64 rng(1);
    std::uniform_real_distribution<double> dist(0.0, 1.0);

    fold::tdigest td;
    for (int i = 0; i < 100'000; ++i) {
        td.add(dist(rng));
    }
    for (double q : { 0.01, 0.1, 0.5, 0.9, 0.99 }) {
        EXPECT_NEAR(td.quantile(q), q, 0.01) << "q = " << q;
    }
    EXPECT_EQ(td.count(), 100'000.0);
}

TEST(TDigest, MergedPartsMatchWhole) {
    std::mt19937_64 rng(2);
    std::normal_distribution<double> dist(1000.0, 150.0);

    fold::tdigest whole;
    std::vector<fold::tdigest> parts(64);
    for (int i = 0; i < 64'000; ++i) {
        const auto v = dist(rng);
        whole.add(v);
        parts[static_cast<std::size_t>(i) % parts.size()].add(v);
    }

    fold::tdigest merged;
    for (const auto& p : parts) {
        merged.merge(p);
    }

    EXPECT_EQ(merged.count(), whole.count());
    for (double q : { 0.05, 0.5, 0.95, 0.99 }) {
        EXPECT_NEAR(merged.quantile(q), whole.quantile(q), 5.0) << "q = " << q;
    }
}

TEST(TDigest, SizeIsBounded) {
    fold::tdigest td(100);
    for (int i = 0; i < 1'000'000; ++i) {
        td.add(static_cast<double>(i % 977));
    }
    EXPECT_LT(td.bytes(), 10'000U);
}

TEST(TDigest, EmptyIsNaN) {
    EXPECT_TRUE(std::isnan(fold::tdigest {}.quantile(0.5)));
}

TEST(TopK, ExactUnderAnyMergeTree) {
    std::mt19937_64 rng(3);
    std::uniform_int_distribution<std::uint32_t> id(1, 2'000);
    std::uniform_int_distribution<std::uint32_t> value(0, 500);    // plenty of ties

    std::map<std::uint32_t, std::uint32_t> best;
    std::vector<fold::top_k<std::uint32_t, std::uint32_t>> shards(37, fold::top_k<std::uint32_t, std::uint32_t>(10));

    for (int i = 0; i < 20'000; ++i) {
        const auto k = id(rng);
        const auto v = value(rng);
        best[k] = std::max(best[k], v);
        shards[static_cast<std::size_t>(i) % shards.size()].offer(k, v);   // same id lands in many shards
    }

    std::vector<std::pair<std::uint32_t, std::uint32_t>> expected(best.begin(), best.end());
    std::ranges::sort(expected, [](const auto& lhs, const auto& rhs) {
        return lhs.second != rhs.second ? lhs.second > rhs.second : lhs.first < rhs.first;
    });
    expected.resize(10);

    for (int round = 0; round < 20; ++round) {
        // Random binary merge tree.
        auto pool = shards;
        while (pool.size() > 1) {
            std::uniform_int_distribution<std::size_t> pick(0, pool.size() - 1);
            auto a = pick(rng);
            auto b = pick(rng);
            if (a == b) { continue; }
            pool[a].merge(pool[b]);
            pool.erase(pool.begin() + static_cast<std::ptrdiff_t>(b));
        }

        const auto& items = pool.front().items();
        ASSERT_EQ(items.size(), expected.size());
        for (std::size_t i = 0; i < items.size(); ++i) {
            EXPECT_EQ(items[i].id, expected[i].first);
            EXPECT_EQ(items[i].value, expected[i].second);
        }
    }
}

TEST(TopK, KeepsBestValuePerId) {
    fold::top_k<int, int> top(3);
    top.offer(1, 10);
    top.offer(1, 5);
    top.offer(1, 20);
    ASSERT_EQ(top.items().size(), 1U);
    EXPECT_EQ(top.items().front().value, 20);
}
