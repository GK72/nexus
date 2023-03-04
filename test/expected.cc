#include <catch2/catch_test_macros.hpp>

#include "nxs/expected.h"

#include <string_view>
#include <type_traits>

TEST_CASE("interface", "[expected]") {
    using Exp = nxs::expected<int>;
    static_assert(std::is_same_v<Exp::value_type, int>);
    static_assert(std::is_same_v<Exp::error_type, nxs::error>);
}

TEST_CASE("trivial types", "[expected]") {
    constexpr auto expect = [](bool condition) -> nxs::expected<int> {
        if (condition) {
            return 1;
        }
        return nxs::unexpected{"Error message"};
    };

    {
        constexpr auto ret = expect(true);

        CHECK(ret);
        CHECK(ret.value() == 1);
        CHECK(*ret == 1);
    }

    {
        constexpr auto ret = expect(false);

        using namespace std::literals::string_view_literals;

        CHECK(!ret);
        CHECK(ret.error() == "Error message"sv);
    }
}

TEST_CASE("non-trivial types", "[expected]") {
    struct S {
        constexpr S() { }
        constexpr ~S() { }
    };

    constexpr auto x = nxs::expected<S>(S{ });
    CHECK(x);
}
