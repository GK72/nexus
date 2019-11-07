#define CATCH_CONFIG_MAIN
#include "catch.hpp"


int add(int a, int b) {
    return a + b;
}

TEST_CASE("Simple Test", "[simple]") {
    REQUIRE(add(1,2) == 3);
}