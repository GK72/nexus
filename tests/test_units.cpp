#include "catch.hpp"

#include "units.h"

using namespace glib::units;

TEST_CASE("Units - bytes", "[units]") {
    SECTION("Conversion: byte to kByte") {
        Unit<int> byte {1024};
        Unit<int, std::ratio<1024>> kByte {1};

        CHECK(byte == kByte);
    }

    SECTION("Conversion: kByte to byte") {
        Unit<int> byte {1024};
        Unit<int, std::ratio<1024>> kByte {1};

        CHECK(kByte == byte);
    }

}

