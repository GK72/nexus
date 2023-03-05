#include <catch2/catch_test_macros.hpp>

#include "nxs/coro.h"
#include "nxs/data.h"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <tuple>

using namespace std::literals::string_view_literals;

nxs::generator<int> incrementer() {
    int i = 0;

    while (true) {
        co_yield i;
        ++i;
    }
}

nxs::generator<int> factorial(int n) {
    co_yield 1;

    if (n == 0 || n == 1) {
        co_return;
    }

    int a = 1;
    for (int i = 1; i <= n; ++i) {
        int s = a * i;
        co_yield s;
        a = s;
    }
}

TEST_CASE("Generator", "[nxs][coro]") {
    auto generate = incrementer();
    CHECK(generate() == 0);
    CHECK(generate() == 1);
    CHECK(generate() == 2);
    CHECK(generate);
}

TEST_CASE("Factorial", "[nxs][coro]") {
    SECTION("n = 0") {
        auto generate = factorial(0);
        int result = 0;
        while (generate) {
            result = generate();
        }
        CHECK(result == 1);
        CHECK_FALSE(generate);
    }

    SECTION("n = 1") {
        auto generate = factorial(1);
        int result = 0;
        while (generate) {
            result = generate();
        }
        CHECK(result == 1);
        CHECK_FALSE(generate);
    }

    SECTION("n = 2") {
        auto generate = factorial(2);
        int result = 0;
        while (generate) {
            result = generate();
        }
        CHECK(result == 2);
        CHECK_FALSE(generate);
    }

    SECTION("n = 3") {
        auto generate = factorial(3);
        int result = 0;
        while (generate) {
            result = generate();
        }
        CHECK(result == 6);
        CHECK_FALSE(generate);
    }
}

TEST_CASE("data_view", "[nxs]") {
    SECTION("view from string") {
        static constexpr auto data = "\x01\x02"sv;
        const auto view_be = nxs::data_view(data);
        const auto view_le = nxs::data_view_le(data);
        CHECK(view_be.as_number<std::uint16_t>(0) == 258);
        CHECK(view_le.as_number<std::uint16_t>(0) == 513);
    }

    SECTION("view from array") {
        static constexpr auto data = std::to_array<unsigned char>({ 0x01, 0x02 });
        const auto view_be = nxs::data_view(data);
        const auto view_le = nxs::data_view_le(data);
        CHECK(view_be.as_number<std::uint16_t>(0) == 258);
        CHECK(view_le.as_number<std::uint16_t>(0) == 513);
    }

    SECTION("interpreting from position") {
        static constexpr auto data = std::to_array<unsigned char>({ 0x00, 0x01, 0x02 });
        const auto view_be = nxs::data_view(data);
        const auto view_le = nxs::data_view_le(data);
        CHECK(view_be.as_number<std::uint16_t>(1) == 258);
        CHECK(view_le.as_number<std::uint16_t>(1) == 513);
    }

    SECTION("interpreting as string") {
        static constexpr auto data = "\x61\x62\x63"sv;
        const auto view = nxs::data_view(data);
        CHECK(view.as_string(0, 3) == "abc");
    }
}

TEST_CASE("data_view subview", "[nxs]") {
    static constexpr auto data = std::to_array<unsigned char>({ 0x01, 0x02, 0x03, 0x04, 0x05 });
    CHECK(nxs::data_view(data).subview(3).as_number<std::uint8_t>(0) == 4);
}

TEST_CASE("data_view dynamic length string", "[nxs]") {
    static constexpr auto data = "\x04\x61\x62\x63\x64\x65"sv;
    CHECK(nxs::data_view(data).as_dyn_string(0) == "abcd");
}

TEST_CASE("data_view hexdump", "[nxs]") {
    static constexpr auto data = "Hello nexus"sv;
    CHECK(fmt::format("{}", nxs::data_view(data).to_hex()) == "\n0000: 48 65 6c 6c 6f 20 6e 65 78 75 73"sv);
}

TEST_CASE("data_view out of bounds access", "[nxs][error]") {
    static constexpr auto data = std::to_array<unsigned char>({ 0x01, 0x02 });
    try {
        std::ignore = nxs::data_view(data).as_number(1, 2);
    }
    catch (const nxs::out_of_data_bounds& ex) {
        CHECK(ex.what() == "Pos: 1, Len: 2, Size: 2 (End: 3)"sv);
    }
}
