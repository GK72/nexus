#include <catch2/catch.hpp>

#include "utility.h"

// template <
//     template <class> class Cont, class T,
//     template <class> class ContOrig, class T2 = T>
// void checkRange(Cont<T>& container, const ContOrig<T2>& orig) {

// }

TEST_CASE("GroupBy", "[utility]") {
    SECTION("Key and value in separate containers") {
        std::vector<int> keys   { 1, 1, 2, 3, 3, 3, 3 };
        std::vector<int> values { 1, 2, 5, 1, 1, 1, 1 };

        auto group = nxs::groupBy(keys, values
            ,[](const auto& key, const auto& prevKey) { return key == prevKey; }
            ,[](auto agg, const auto& value)          { return agg += value; }
        );
        CHECK(group[0].first  == 1);
        CHECK(group[0].second == 3);

        CHECK(group[1].first  == 2);
        CHECK(group[1].second == 5);

        CHECK(group[2].first  == 3);
        CHECK(group[2].second == 4);
    }
}

TEST_CASE("String split", "[string]") {
    SECTION("Empty string") {
        CHECK(nxs::strSplit("", " ")[0] == "");
    }

    SECTION("One char") {
        CHECK(nxs::strSplit("a", " ")[0] == "a");
    }

    SECTION("Two chars separated") {
        auto result = nxs::strSplit("a b", " ");
        CHECK(result[0] == "a");
        CHECK(result[1] == "b");
    }

    SECTION("Three chars separated") {
        auto result = nxs::strSplit("a b c", " ");
        CHECK(result[0] == "a");
        CHECK(result[1] == "b");
        CHECK(result[2] == "c");
    }

    SECTION("Four chars separated") {
        auto result = nxs::strSplit("a b c d", " ");
        CHECK(result[0] == "a");
        CHECK(result[1] == "b");
        CHECK(result[2] == "c");
        CHECK(result[3] == "d");
    }

    SECTION("Two words separated") {
        auto result = nxs::strSplit("bla blabla", " ");
        CHECK(result[0] == "bla");
        CHECK(result[1] == "blabla");
    }

    SECTION("Three words separated") {
        auto result = nxs::strSplit("bla blabla blaaaa", " ");
        CHECK(result[0] == "bla");
        CHECK(result[1] == "blabla");
        CHECK(result[2] == "blaaaa");
    }

    SECTION("Three words separated - multichar split") {
        auto result = nxs::strSplit("bla--blabla--blaaaa", "--");
        CHECK(result[0] == "bla");
        CHECK(result[1] == "blabla");
        CHECK(result[2] == "blaaaa");
    }
}
