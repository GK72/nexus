#include "catch.hpp"

#include "units.h"

using namespace glib::units;

TEST_CASE("Units/Bytes: Implicit conversions", "[units]") {
    SECTION("Implicit conversion to smaller") {
        auto bytes = byte{ 1 };
        auto bits  = bit { bytes };
        CHECK(bits == bytes);
    }

    SECTION("Explicit conversion to larger") {
        // TODO: make it explicit, disallow implicit up casting
        auto bits  = bit { 8 };
        auto bytes = byte{ bits };
        CHECK(bits == bytes);
    }
}

TEST_CASE("Units/Bytes: Helper types (equality)", "[units]") {
    SECTION("Conversion: bit - byte") {
        auto lhs = bit  { 8 };
        auto rhs = byte { 1 };
        CHECK(lhs == rhs);
        CHECK(rhs == lhs);
    }

    SECTION("Conversion: byte - kByte") {
        auto lhs = byte  { 1024 };
        auto rhs = kByte { 1 };
        CHECK(lhs == rhs);
        CHECK(rhs == lhs);
    }

    SECTION("Conversion: kByte - MByte") {
        auto lhs = kByte { 1024 };
        auto rhs = MByte { 1 };
        CHECK(lhs == rhs);
        CHECK(rhs == lhs);
    }

    SECTION("Conversion: MByte - GByte") {
        auto lhs = MByte{ 1024 };
        auto rhs = GByte{ 1 };
        CHECK(lhs == rhs);
        CHECK(rhs == lhs);
    }

    SECTION("Conversion: GByte - TByte") {
        auto lhs = GByte{ 1024 };
        auto rhs = TByte{ 1 };
        CHECK(lhs == rhs);
        CHECK(rhs == lhs);
    }

    SECTION("Conversion: kByte - GByte") {
        auto lhs = kByte{ 1024 * 1024 };
        auto rhs = GByte{ 1 };
        CHECK(lhs == rhs);
        CHECK(rhs == lhs);
    }
}

TEST_CASE("Units/Bytes: Relational operators", "[units]") {
    SECTION("Common type") {
        SECTION("Operator !=") {
            auto lhs = byte{ 1 };
            auto rhs = byte{ 2 };
            CHECK(lhs != rhs);
            CHECK(rhs != lhs);
        }

        SECTION("Operator <") {
            auto lhs = byte{ 1 };
            auto rhs = byte{ 2 };
            CHECK(lhs < rhs);
        }

        SECTION("Operator >") {
            auto lhs = byte{ 2 };
            auto rhs = byte{ 1 };
            CHECK(lhs > rhs);
        }

        SECTION("Operator <=") {
            auto lhs = byte{ 1 };
            auto rhs = byte{ 2 };
            CHECK(lhs < rhs);
            CHECK(lhs <= rhs);
            CHECK(lhs <= lhs);
        }

        SECTION("Operator >=") {
            auto lhs = byte{ 2 };
            auto rhs = byte{ 1 };
            CHECK(lhs > rhs);
            CHECK(lhs >= rhs);
            CHECK(lhs >= lhs);
        }
    }

    SECTION("Different types") {
        SECTION("Operator !=") {
            auto lhs = bit { 1 };
            auto rhs = byte{ 1 };
            CHECK(lhs != rhs);
            CHECK(rhs != lhs);
        }

        SECTION("Operator <") {
            auto lhs = bit { 1 };
            auto rhs = byte{ 1 };
            CHECK(lhs < rhs);
        }

        SECTION("Operator >") {
            auto lhs = bit { 16 };
            auto rhs = byte{ 1 };
            CHECK(lhs > rhs);
        }

        SECTION("Operator > (one-way-only conversion)") {
            auto lhs = bit { 9 };
            auto rhs = byte{ 1 };
            CHECK(lhs > rhs);
        }

        SECTION("Operator <=") {
            auto lhs  = bit { 1 };
            auto lhs2 = bit { 8 };
            auto rhs  = byte{ 1 };
            CHECK(lhs  < rhs);
            CHECK(lhs  <= rhs);
            CHECK(lhs2 <= rhs);
        }

        SECTION("Operator >=") {
            auto lhs  = byte{ 1 };
            auto rhs  = bit { 7 };
            auto rhs2 = bit { 8 };
            CHECK(lhs > rhs);
            CHECK(lhs >= rhs);
            CHECK(lhs >= rhs2);
        }
    }
}

TEST_CASE("Units/Bytes: Arithmetic operators", "[units]") {
    SECTION("Common type") {
        SECTION("Addition") {
            auto lhs = byte{ 1 };
            auto rhs = byte{ 1 };
            CHECK(lhs + rhs == byte{ 2 });
        }

        SECTION("Subtraction") {
            // TODO
        }

        // TODO ...
    }

    SECTION("Different types") {
        // TODO ...
    }
}

TEST_CASE("Units/Bytes: Literals", "[units]") {
    using namespace literals;
    SECTION("Base") {
        auto x = 8_bit;
        auto y = 1_byte;
        CHECK(x == y);
    }
}

