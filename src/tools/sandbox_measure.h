#pragma once

#include <algorithm>
#include <vector>

#include "utility.h"

namespace nxs::sandbox {

template <class T> using Container = std::vector<T>;

constexpr size_t DATA_SIZE_A    = 400;
constexpr size_t CONTAINER_SIZE = 100'000;

template <size_t SIZE>
struct Data {
    char value[SIZE];       // NOLINT
};

template <class T>
auto processComplex(const T& range) {
    T result;

    for (size_t i = 0; const auto& x : range) {
        if (x.value[0] == 0) {
            result.push_back(Data<DATA_SIZE_A>{static_cast<char>(x.value[0] ^ x.value[1])});
        }
        ++i;
    }

    nxs::print("", "Source container size: ", range.size());
    nxs::print("", "Result container size: ", result.size());

    return range;
}

template <class T>
auto filter(const T& range) {
    T result;

    std::copy_if(std::begin(range), std::end(range), std::back_inserter(result),
        [](const auto& x) { return x.value[0] == 0; }
    );

    nxs::print("", "Source container size: ", range.size());
    nxs::print("", "Result container size: ", result.size());

    return range;
}

template <class T>
auto process(const T& range) {
    T result;

    std::transform(std::begin(range), std::end(range), std::back_inserter(result),
        [](const auto& x) { return Data<DATA_SIZE_A>{static_cast<char>(x.value[0] ^ x.value[1])}; }
    );

    return result;
}

template <class T>
auto processTransformIf(const T& range) {
    return transform_if(
            range,
            [](const auto& x) { return Data<DATA_SIZE_A>{static_cast<char>(x.value[0] ^ x.value[1])}; },
            [](const auto& x) { return x.value[0] == 0; }
        );
}



inline void run() {
    Container<Data<DATA_SIZE_A>> container(CONTAINER_SIZE);

    // auto result = processComplex(container);
    // auto result = process(filter(container));
    auto result = processTransformIf(container);

}

} // namespace nxs::sandbox
