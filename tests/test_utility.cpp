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
