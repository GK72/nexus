#include <catch2/catch.hpp>

#include "datetime.h"

namespace nxs::test {

TEST_CASE("Date - Construction", "[date]") {
    SECTION("From string") {
        auto date = nxs::datetime::Date("2020.05.31");
        CHECK(date.year()  == 2020);
        CHECK(date.month() == 5);
        CHECK(date.day()   == 31);
    }

    SECTION("From string - trailing char") {
        auto date = nxs::datetime::Date("2020.05.31.");
        CHECK(date.year()  == 2020);
        CHECK(date.month() == 5);
        CHECK(date.day()   == 31);
    }

    // TODO: more tests
}

} // namespace nxs::test
