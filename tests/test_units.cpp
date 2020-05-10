#include "catch.hpp"

#include "units.h"

using namespace glib::units;

TEST_CASE("Units - bytes", "[units]") {
    SECTION("Conversion") {
        Size<int> byte {1};
        Size<int, std::ratio<1024>> kByte {1};

        CHECK(byte == kByte);
    }

}

