#include <catch2/catch_test_macros.hpp>

#include "nxs/flat_map.h"

#include <array>
#include <type_traits>
#include <string_view>
#include <vector>

using IntMap = nxs::flat_map<int, int>;

TEST_CASE("interface", "[flat_map], [data]") {
    using Map = nxs::flat_map<int, char>;
    static_assert(std::is_same_v<Map::iterator_pair::key_iterator, std::vector<int>::iterator>);
    static_assert(std::is_same_v<Map::iterator_pair::value_iterator, std::vector<char>::iterator>);
}

TEST_CASE("initializer list", "[flat_map], [data]") {
    const auto map = IntMap({
        { 2, 4 },
        { 3, 6 },
    });

    CHECK(map.size() == 2);
    CHECK(map.keys() == std::vector{ 2, 3 });
}

TEST_CASE("observers", "[flat_map], [data]") {
    auto map = IntMap();

    CHECK(map.size() == 0);
    CHECK(map.empty());
    CHECK(map.keys().empty());
    CHECK(map.values().empty());
}

TEST_CASE("insertion", "[flat_map], [data]") {
    auto map = IntMap();
    map.insert({2, 4});
    map.insert({1, 2});

    CHECK(map[1] == 2);
    CHECK(map[2] == 4);

    CHECK(map.values() == std::vector<int>({ 2, 4 }));

    map.insert({1, 3});
    CHECK(map[1] == 2);
}

TEST_CASE("iterators", "[flat_map], [data]") {
    static_assert(std::is_same_v<IntMap::iterator::iterator_category, std::random_access_iterator_tag>);

    auto map = IntMap();

    auto begin = std::begin(map);
    auto end = std::end(map);
    auto rbegin = std::rbegin(map);
    auto rend = std::rend(map);

    const auto cbegin = std::begin(map);
    const auto cend = std::end(map);
    const auto ccbegin = std::cbegin(map);
    const auto ccend = std::cend(map);

    const auto crbegin = std::rbegin(map);
    const auto crend = std::rend(map);
    const auto ccrbegin = std::crbegin(map);
    const auto ccrend = std::crend(map);

    CHECK(begin == end);
    CHECK(cbegin == cend);
    CHECK(ccbegin == ccend);
    CHECK(rbegin == rend);
    CHECK(crbegin == crend);
    CHECK(ccrbegin == ccrend);

    for ([[maybe_unused]] const auto& [key, value] : map) {
    }
}

TEST_CASE("subscript operator", "[flat_map], [data]") {
    auto map = IntMap();

    map[4] = 2;
    map[2] = 4;
    map[1] = 3;
    map[3] = 1;

    CHECK(map[1] == 3);
    CHECK(map[2] == 4);
    CHECK(map[3] == 1);
    CHECK(map[4] == 2);

    map[4] = 11;
    CHECK(map[4] == 11);
}

TEST_CASE("constexpr", "[flat_map], [data]") {
    using Dict = nxs::static_map<std::string_view, std::string_view, 2>;

    constexpr auto map = Dict({
        { "a", "something" },
        { "b", "another something" }
    });

    CHECK(map.size() == 2);
    CHECK(map.keys() == std::array<std::string_view, 2>({ "a", "b" }));
}
