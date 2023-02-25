#include <catch2/catch_test_macros.hpp>

#include "units.h"

using namespace nxs::units;
using namespace literals;

TEST_CASE("Units/Bytes: Conversions", "[units]") {
    SECTION("Implicit conversion to smaller") {
        CHECK(bit(1_byte) ==    8_bit);
        CHECK(bit(1_kB)   == 8192_bit);
    }

    SECTION("Explicit conversion to larger") {
        CHECK(unit_cast<byte>( 9_bit) == 1_byte);
        CHECK(unit_cast<byte>(15_bit) == 1_byte);
        CHECK(unit_cast<byte>(16_bit) == 2_byte);
    }
    SECTION("Units_Bytes, Convertible") {
        static_assert(!std::is_convertible_v<byte, kByte>);
        static_assert(std::is_convertible_v<kByte, byte>);
    }

    SECTION("Implicit conversion between representations")
        CHECK(Unit<int>(8) == Unit<long>(8));

        CHECK( (Unit<int, std::ratio<1000>>   (8)) == (Unit<long,   std::ratio<1, 1000>>(8'000'000)) );
        CHECK( (Unit<int, std::ratio<1000>>   (8)) == (Unit<double, std::ratio<1, 1000>>(8e6)      ) );
        CHECK( (Unit<int, std::ratio<1, 1000>>(8)) == (Unit<double>                     (8e-3)     ) );
}

TEST_CASE("Units/Bytes: Helper types", "[units]") {
    CHECK(   8_bit  == 1_byte);
    CHECK(1024_byte == 1_kB);
    CHECK(1024_kB   == 1_MB);
    CHECK(1024_MB   == 1_GB);
    CHECK(1024_GB   == 1_TB);

    CHECK(1_byte == 8_bit);
    CHECK(1_kB   == 1024_byte);
    CHECK(1_MB   == 1024_kB);
    CHECK(1_GB   == 1024_MB);
    CHECK(1_TB   == 1024_GB);
}

TEST_CASE("Units/Bytes: Relational operators", "[units]") {
    SECTION("Common type") {
        CHECK(1_byte != 2_byte);
        CHECK(1_byte <  2_byte);
        CHECK(1_byte <= 2_byte);
        CHECK(1_byte <= 1_byte);

        CHECK(2_byte != 1_byte);
        CHECK(2_byte >  1_byte);
        CHECK(2_byte >= 1_byte);
        CHECK(2_byte >= 2_byte);
    }

    SECTION("Different types") {
        SECTION("bit - byte") {
            CHECK(1_bit  != 1_byte);
            CHECK(1_bit  <  1_byte);
            CHECK(1_bit  <= 1_byte);
            CHECK(8_bit  <= 1_byte);

            CHECK(1_byte != 1_bit);
            CHECK(1_byte >  1_bit);
            CHECK(1_byte >= 1_bit);
            CHECK(1_byte >= 8_bit);
        }

        SECTION("9 bit - 1 byte") {
            CHECK(1_byte != 9_bit);
            CHECK(1_byte <  9_bit);
            CHECK(1_byte <= 9_bit);

            CHECK(9_bit != 1_byte);
            CHECK(9_bit >= 1_byte);
            CHECK(9_bit >  1_byte);
        }
    }
}

TEST_CASE("Units/Bytes: Arithmetic operators", "[units]") {
    SECTION("Multiplication and divison") {
        CHECK(12_byte * 2  == 24_byte);
        CHECK( 2 * 3_byte  ==  6_byte);
        CHECK(12_byte  / 2 ==  6_byte);
        CHECK( 4_byte  / 2 == 16_bit);
    }

    SECTION("Modulo division") {
        CHECK(14_bit  % 8     == 6_bit);
        CHECK( 4_byte % 6_bit == 2_bit);
    }

    SECTION("Addition and subtraction") {
        CHECK(12_byte + 3_byte == 15_byte);
        CHECK(12_byte - 3_byte ==  9_byte);
        CHECK(12_byte + 2_bit  == 98_bit);
        CHECK(12_byte - 2_bit  == 94_bit);
    }

    SECTION("Member operators") {
        auto x = 8_byte;

        CHECK( (x += 2_byte) == 10_byte);
        CHECK( (x -= 2_byte) ==  8_byte);
        CHECK( (x *= 2)      == 16_byte);
        CHECK( (x /= 4)      ==  4_byte);

        CHECK(++x == 5_byte);
        CHECK(--x == 4_byte);

        CHECK(x++ == 4_byte);
        CHECK(x   == 5_byte);
        CHECK(x-- == 5_byte);
        CHECK(x   == 4_byte);
    }
}

TEST_CASE("Units/Bytes: Member functions", "[units]") {
    auto x = 8_byte;

    CHECK(x.count() == 8);

    CHECK(byte::zero().count() == 0);
    CHECK(byte::max().count()  == std::numeric_limits<long long>::max());
    CHECK(byte::min().count()  == std::numeric_limits<long long>::min());

    CHECK(Unit<double>::min().count() == std::numeric_limits<double>::lowest());
}
