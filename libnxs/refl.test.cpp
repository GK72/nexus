#include <libnxs/refl.hpp>

#include <libnova/test_utils.hpp>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <string_view>

using namespace std::string_view_literals;

namespace ann {

inline constexpr struct {} type_annotation{};
inline constexpr struct {} value_annotation{};

} // namespace ann

TEST(Refl, HasAnnotation) {
    struct [[=ann::type_annotation]] test {
        int x;
    };

    [[=ann::value_annotation]] auto x = test{ };

    static_assert(nxs::refl::has_annotation(^^test, ^^ann::type_annotation));
    static_assert(nxs::refl::has_annotation(^^x, ^^ann::value_annotation));
    static_assert(nxs::refl::has_annotation(std::meta::type_of(^^x), ^^ann::type_annotation));
}

TEST(Refl, EnumToString) {
    enum class enum_1 {
        enum_a,
        enum_b
    };

    constexpr auto x = enum_1::enum_a;
    static_assert(nxs::refl::enum_to_string(x) == "enum_a");
}

TEST(Refl, StringToEnum) {
    enum class enum_2 {
        enum_a,
        enum_b
    };

    constexpr enum_2 x = nxs::refl::string_to_enum<enum_2>("enum_b");
    static_assert(x == enum_2::enum_b);
    EXPECT_THROWN_MESSAGE(nxs::refl::string_to_enum<enum_2>("enum_c"), "Invalid enumeration: enum_c");
}

TEST(Refl, Formatter) {
    struct [[=nxs::ann::fmt_debug]] point2d {
        int x;
        int y;
    };

    constexpr auto p = point2d{ .x = 6, .y = 9 };
    EXPECT_EQ(fmt::format("{}", p), "point2d{ .x=6, .y=9 }");
}

auto add(int x, int y) -> int {
    return x + y;
}

TEST(Refl, FunctionTrace) {
    auto tracer = nxs::refl::function_tracer();
    std::string sink;
    tracer.logger = [&sink](std::string_view msg) { sink = msg; };

    tracer.trace<^^add>(6, 9);
    EXPECT_EQ(sink, "x=6 y=9 return=15"sv);
}
