#include <benchmark/benchmark.h>

#include <array>
#include <algorithm>
#include <list>
#include <vector>

template <class T>
static void sortList(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        std::list<int> list(state.range(0));
        state.ResumeTiming();

        list.sort();
        benchmark::DoNotOptimize(list);
    }
}

template <class T>
static void sortVec(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        std::vector<T> vec(state.range(0));
        state.ResumeTiming();

        std::sort(std::begin(vec), std::end(vec));
        benchmark::DoNotOptimize(vec.data());
    }
}

template <size_t SIZE>
struct S {
    char x[SIZE];

    friend bool operator<(const S& lhs, const S& rhs) { return lhs.x[0] < rhs.x[0]; }
};


BENCHMARK_TEMPLATE(sortList, int)->RangeMultiplier(2)->Range(2, 2 << 10);
BENCHMARK_TEMPLATE(sortVec , int)->RangeMultiplier(2)->Range(2, 2 << 10);
BENCHMARK_TEMPLATE(sortList, S<8>)->RangeMultiplier(2)->Range(2 << 8, 2 << 10);
BENCHMARK_TEMPLATE(sortVec , S<8>)->RangeMultiplier(2)->Range(2 << 8, 2 << 10);
BENCHMARK_TEMPLATE(sortList, S<128>)->Arg(1024);
BENCHMARK_TEMPLATE(sortVec , S<128>)->Arg(1024);
BENCHMARK_TEMPLATE(sortList, S<256>)->Arg(1024);
BENCHMARK_TEMPLATE(sortVec , S<256>)->Arg(1024);
BENCHMARK_TEMPLATE(sortList, S<512>)->Arg(1024);
BENCHMARK_TEMPLATE(sortVec , S<512>)->Arg(1024);
BENCHMARK_TEMPLATE(sortList, S<1024>)->Arg(1024);
BENCHMARK_TEMPLATE(sortVec , S<1024>)->Arg(1024);

BENCHMARK_MAIN();
