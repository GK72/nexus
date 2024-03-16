#include <benchmark/benchmark.h>

#include <limits>
#include <nova/random.h>

static void add(benchmark::State& state) {
    auto a = nova::random().number();
    auto b = nova::random().number();

    for (auto _ : state) {
        auto x = a + b;
        benchmark::DoNotOptimize(x);
    }
}

static void mul(benchmark::State& state) {
    auto a = nova::random().number();
    auto b = nova::random().number();

    for (auto _ : state) {
        auto x = a * b;
        benchmark::DoNotOptimize(x);
    }
}

static void div(benchmark::State& state) {
    auto a = nova::random().number(nova::range<long long>{ 1LL, std::numeric_limits<long long>::max() });
    auto b = nova::random().number(nova::range<long long>{ 1LL, std::numeric_limits<long long>::max() });

    for (auto _ : state) {
        auto x = a / b;
        benchmark::DoNotOptimize(x);
    }
}

static void sqrt(benchmark::State& state) {
    auto a = nova::random().number();

    for (auto _ : state) {
        auto x = std::sqrt(a);
        benchmark::DoNotOptimize(x);
    }
}

static void sin(benchmark::State& state) {
    auto a = nova::random().number();

    for (auto _ : state) {
        auto x = std::sin(a);
        benchmark::DoNotOptimize(x);
    }
}

static void cos(benchmark::State& state) {
    auto a = nova::random().number();

    for (auto _ : state) {
        auto x = std::cos(a);
        benchmark::DoNotOptimize(x);
    }
}

BENCHMARK(add);
BENCHMARK(mul);
BENCHMARK(div);
BENCHMARK(sqrt);
BENCHMARK(sin);
BENCHMARK(cos);

BENCHMARK_MAIN();
