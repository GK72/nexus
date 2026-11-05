/**
 * Part of Nexus Library
 *
 * Fixed-size, mergeable summaries for values that have no exact compact
 * partial (distinct counts, quantiles) plus an exact top-k.
 *
 * Every type here is a valid building block for an `aggregation::partial`:
 * default-constructible, copyable, and with a `merge` whose result does not
 * depend on how the inputs were grouped.
 *
 *   - hyperloglog: merge is register-wise max, so it is also idempotent and
 *     the merged registers are identical regardless of order.
 *   - tdigest:     merge is order-insensitive up to the digest's error bound;
 *     the exact centroids may differ between topologies.
 *   - top_k:       exact top-k of the best value per id, under a strict total
 *     order (value desc, id asc), so ties cannot make it order-dependent.
 *
 * @author      Gábor Krisztián Girhiny
 * @coauthor    Claude Opus 5.5 (Claude)
 * @date        2026-11-05
 */

#pragma once

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <numbers>
#include <stdexcept>
#include <utility>
#include <vector>

namespace fold {

/// SplitMix64 finalizer; good enough to spread sequential ids over 64 bits.
constexpr auto mix64(std::uint64_t x) -> std::uint64_t {
    x += 0x9E3779B97F4A7C15ULL;
    x = (x ^ (x >> 30U)) * 0xBF58476D1CE4E5B9ULL;
    x = (x ^ (x >> 27U)) * 0x94D049BB133111EBULL;
    return x ^ (x >> 31U);
}

/**
 * @brief   Distinct-count estimator. Relative error is about 1.04 / sqrt(2^p).
 *
 * Starts sparse (only touched registers are stored), so lifting a single
 * event is cheap; switches to 2^p dense byte registers once the sparse form
 * would be larger. Representation never affects the estimate or equality.
 */
class hyperloglog {
public:
    explicit hyperloglog(unsigned precision = 14)
        : m_precision(precision)
    {
        if (precision < 4 || precision > 18) {
            throw std::invalid_argument("hyperloglog: precision must be in [4, 18]");
        }
    }

    void add(std::uint64_t key) {
        const auto hash  = mix64(key);
        const auto index = static_cast<std::uint32_t>(hash >> (64U - m_precision));
        const auto rest  = (hash << m_precision) | (std::uint64_t { 1 } << (m_precision - 1));
        const auto rank  = static_cast<std::uint8_t>(std::countl_zero(rest) + 1);
        set(index, rank);
    }

    void merge(const hyperloglog& other) {
        if (other.m_precision != m_precision) {
            throw std::invalid_argument("hyperloglog: cannot merge different precisions");
        }

        if (other.is_dense()) {
            densify();
            for (std::size_t i = 0; i < m_dense.size(); ++i) {
                m_dense[i] = std::max(m_dense[i], other.m_dense[i]);
            }
            return;
        }

        for (const auto& [index, rank] : other.m_sparse) {
            set(index, rank);
        }
    }

    /**
     * Ertl's improved estimator ("New cardinality estimation algorithms for
     * HyperLogLog sketches", 2017): unbiased over the whole range without
     * the empirical bias tables of HLL++ or a linear-counting switch.
     */
    [[nodiscard]] auto estimate() const -> double {
        const auto q = 64U - m_precision;                    // max rank is q + 1
        std::vector<double> histogram(q + 2, 0.0);
        for_each_register([&](std::uint8_t reg) { histogram[reg] += 1.0; });

        const auto m = static_cast<double>(registers());
        if (histogram[0] == m) {
            return 0.0;
        }

        auto z = m * tau(1.0 - histogram[q + 1] / m);
        for (auto k = q; k >= 1; --k) {
            z = 0.5 * (z + histogram[k]);
        }
        z += m * sigma(histogram[0] / m);

        return m * m / (2.0 * std::numbers::ln2 * z);
    }

    [[nodiscard]] auto is_dense() const -> bool { return not m_dense.empty(); }

    [[nodiscard]] auto bytes() const -> std::size_t {
        return is_dense() ? m_dense.size() : m_sparse.size() * sizeof(sparse_entry);
    }

    auto operator==(const hyperloglog& other) const -> bool {
        if (m_precision != other.m_precision) {
            return false;
        }
        return dense_copy() == other.dense_copy();
    }

private:
    struct sparse_entry {
        std::uint32_t index;
        std::uint8_t rank;
    };

    unsigned m_precision;
    std::vector<sparse_entry> m_sparse;     ///< sorted by index; used while m_dense is empty
    std::vector<std::uint8_t> m_dense;

    [[nodiscard]] auto registers() const -> std::size_t { return std::size_t { 1 } << m_precision; }

    static auto sigma(double x) -> double {
        if (x == 1.0) {
            return std::numeric_limits<double>::infinity();
        }
        double y = 1.0;
        double z = x;
        for (double prev = -1.0; z != prev;) {
            x *= x;
            prev = z;
            z += x * y;
            y += y;
        }
        return z;
    }

    static auto tau(double x) -> double {
        if (x == 0.0 || x == 1.0) {
            return 0.0;
        }
        double y = 1.0;
        double z = 1.0 - x;
        for (double prev = -1.0; z != prev;) {
            x = std::sqrt(x);
            prev = z;
            y *= 0.5;
            z -= (1.0 - x) * (1.0 - x) * y;
        }
        return z / 3.0;
    }

    void set(std::uint32_t index, std::uint8_t rank) {
        if (is_dense()) {
            m_dense[index] = std::max(m_dense[index], rank);
            return;
        }

        auto it = std::ranges::lower_bound(m_sparse, index, {}, &sparse_entry::index);
        if (it != m_sparse.end() && it->index == index) {
            it->rank = std::max(it->rank, rank);
            return;
        }
        m_sparse.insert(it, { index, rank });

        if (m_sparse.size() * sizeof(sparse_entry) > registers()) {
            densify();
        }
    }

    void densify() {
        if (is_dense()) {
            return;
        }
        m_dense = dense_copy();
        m_sparse.clear();
        m_sparse.shrink_to_fit();
    }

    [[nodiscard]] auto dense_copy() const -> std::vector<std::uint8_t> {
        if (is_dense()) {
            return m_dense;
        }
        std::vector<std::uint8_t> regs(registers(), 0);
        for (const auto& [index, rank] : m_sparse) {
            regs[index] = rank;
        }
        return regs;
    }

    void for_each_register(auto&& fn) const {
        if (is_dense()) {
            for (auto reg : m_dense) { fn(reg); }
            return;
        }
        for (std::size_t i = m_sparse.size(); i < registers(); ++i) { fn(std::uint8_t { 0 }); }
        for (const auto& e : m_sparse) { fn(e.rank); }
    }
};

/**
 * @brief   Merging t-digest (Dunning) with the k1 (arcsine) scale function.
 *
 * The number of centroids is bounded by roughly `compression`, independent
 * of how many values were added. Accuracy is best at the tails.
 */
class tdigest {
public:
    struct centroid {
        double mean;
        double weight;
    };

    explicit tdigest(double compression = 100.0)
        : m_compression(compression)
    {}

    void add(double value, double weight = 1.0) {
        m_buffer.push_back({ value, weight });
        m_min = std::min(m_min, value);
        m_max = std::max(m_max, value);
        if (m_buffer.size() >= buffer_limit()) {
            compress();
        }
    }

    void merge(const tdigest& other) {
        m_buffer.insert(m_buffer.end(), other.m_centroids.begin(), other.m_centroids.end());
        m_buffer.insert(m_buffer.end(), other.m_buffer.begin(), other.m_buffer.end());
        m_min = std::min(m_min, other.m_min);
        m_max = std::max(m_max, other.m_max);
        if (m_buffer.size() >= buffer_limit()) {
            compress();
        }
    }

    [[nodiscard]] auto count() const -> double {
        double total = 0.0;
        for (const auto& c : m_centroids) { total += c.weight; }
        for (const auto& c : m_buffer)    { total += c.weight; }
        return total;
    }

    [[nodiscard]] auto quantile(double q) const -> double {
        if (not m_buffer.empty()) {
            auto copy = *this;
            copy.compress();
            return copy.quantile(q);
        }

        if (m_centroids.empty()) {
            return std::numeric_limits<double>::quiet_NaN();
        }
        if (m_centroids.size() == 1) {
            return m_centroids.front().mean;
        }

        const auto total  = count();
        const auto target = std::clamp(q, 0.0, 1.0) * total;

        const auto& first = m_centroids.front();
        if (target < first.weight / 2.0) {
            return lerp(m_min, first.mean, target / (first.weight / 2.0));
        }

        double cumulative = 0.0;
        for (std::size_t i = 0; i + 1 < m_centroids.size(); ++i) {
            const auto& lhs = m_centroids[i];
            const auto& rhs = m_centroids[i + 1];
            const auto mid_lhs = cumulative + lhs.weight / 2.0;
            const auto mid_rhs = cumulative + lhs.weight + rhs.weight / 2.0;
            if (target < mid_rhs) {
                return lerp(lhs.mean, rhs.mean, (target - mid_lhs) / (mid_rhs - mid_lhs));
            }
            cumulative += lhs.weight;
        }

        const auto& last = m_centroids.back();
        const auto mid_last = total - last.weight / 2.0;
        return lerp(last.mean, m_max, (target - mid_last) / (last.weight / 2.0));
    }

    [[nodiscard]] auto centroids() const -> std::size_t { return m_centroids.size() + m_buffer.size(); }
    [[nodiscard]] auto bytes() const -> std::size_t { return centroids() * sizeof(centroid); }

private:
    double m_compression;
    std::vector<centroid> m_centroids;
    std::vector<centroid> m_buffer;
    double m_min = std::numeric_limits<double>::infinity();
    double m_max = -std::numeric_limits<double>::infinity();

    [[nodiscard]] auto buffer_limit() const -> std::size_t {
        return static_cast<std::size_t>(m_compression * 5.0);
    }

    static auto lerp(double lhs, double rhs, double t) -> double {
        return lhs + std::clamp(t, 0.0, 1.0) * (rhs - lhs);
    }

    [[nodiscard]] auto scale(double q) const -> double {
        return m_compression / (2.0 * std::numbers::pi) * std::asin(2.0 * std::clamp(q, 0.0, 1.0) - 1.0);
    }

    [[nodiscard]] auto scale_inverse(double k) const -> double {
        const auto arg = 2.0 * std::numbers::pi * k / m_compression;
        if (arg >= std::numbers::pi / 2.0) {
            return 1.0;
        }
        return (std::sin(arg) + 1.0) / 2.0;
    }

    void compress() {
        if (m_buffer.empty()) {
            return;
        }

        std::vector<centroid> all;
        all.reserve(m_buffer.size() + m_centroids.size());
        all.insert(all.end(), m_buffer.begin(), m_buffer.end());
        all.insert(all.end(), m_centroids.begin(), m_centroids.end());
        m_buffer.clear();
        std::ranges::sort(all, [](const centroid& lhs, const centroid& rhs) {
            return lhs.mean < rhs.mean;
        });

        double total = 0.0;
        for (const auto& c : all) { total += c.weight; }

        std::vector<centroid> result;
        result.reserve(static_cast<std::size_t>(m_compression * 2.0));

        auto current   = all.front();
        double before  = 0.0;
        double q_limit = scale_inverse(scale(0.0) + 1.0);

        for (std::size_t i = 1; i < all.size(); ++i) {
            const auto& next = all[i];
            const auto q = (before + current.weight + next.weight) / total;
            if (q <= q_limit) {
                const auto weight = current.weight + next.weight;
                current.mean += (next.mean - current.mean) * next.weight / weight;
                current.weight = weight;
            }
            else {
                result.push_back(current);
                before += current.weight;
                q_limit = scale_inverse(scale(before / total) + 1.0);
                current = next;
            }
        }
        result.push_back(current);

        m_centroids = std::move(result);
    }
};

/**
 * @brief   Exact top-k over the *best* value per id.
 *
 * Exact under any merge tree: if an id belongs to the global top-k, fewer
 * than k other ids beat it anywhere, so no intermediate merge can drop it.
 * Seeing the same id in several subtrees (e.g. a reconnecting player) is
 * harmless, the best value wins.
 */
template <typename Id, typename Value>
class top_k {
public:
    struct entry {
        Id id;
        Value value;
        auto operator==(const entry&) const -> bool = default;
    };

    explicit top_k(std::size_t k = 10)
        : m_k(k)
    {}

    void offer(const Id& id, const Value& value) {
        auto it = std::ranges::find(m_entries, id, &entry::id);
        if (it != m_entries.end()) {
            if (value <= it->value) {
                return;
            }
            it->value = value;
        }
        else {
            m_entries.push_back({ id, value });
        }

        std::ranges::sort(m_entries, before);
        if (m_entries.size() > m_k) {
            m_entries.resize(m_k);
        }
    }

    void merge(const top_k& other) {
        for (const auto& e : other.m_entries) {
            offer(e.id, e.value);
        }
    }

    [[nodiscard]] auto items() const -> const std::vector<entry>& { return m_entries; }
    [[nodiscard]] auto bytes() const -> std::size_t { return m_k * sizeof(entry); }

    auto operator==(const top_k&) const -> bool = default;

private:
    std::size_t m_k;
    std::vector<entry> m_entries;   // sorted by `before`, size <= k

    static auto before(const entry& lhs, const entry& rhs) -> bool {
        if (lhs.value != rhs.value) {
            return lhs.value > rhs.value;
        }
        return lhs.id < rhs.id;
    }
};

} // namespace fold
