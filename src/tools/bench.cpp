#include <benchmark/benchmark.h>

#include <array>

constexpr size_t SIZE = 1'000;

static void withCond(benchmark::State& state) {
    auto vec = std::array<int, SIZE>();
    bool c = true;
    int sum = 0;

    for (auto _ : state) {
        for (const auto& x : vec) {
            if (c) {
                sum += x;
            }
        }
    }
}

BENCHMARK(withCond);

static void withoutCond(benchmark::State& state) {
    auto vec = std::array<int, SIZE>();
    int sum = 0;

    for (auto _ : state) {
        for (const auto& x : vec) {
            sum += x;
        }
    }
}

BENCHMARK(withoutCond);

BENCHMARK_MAIN();
