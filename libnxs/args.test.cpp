#include <libnxs/args.hpp>

#include <libnova/test_utils.hpp>

#include <gtest/gtest.h>

#include <span>

struct cli_args {
    std::string file;
    int count = 1;

    auto operator<=>(const cli_args&) const = default;
};

TEST(Args, Basic) {
    auto args = cli_args {
        .file = "/path/to/something",
        .count = 2
    };

    auto result = nxs::args::parse<cli_args>(
        std::span<const std::string_view>({
            "--file", "/path/to/something",
            "--count", "2",
            "--unknown", "don't care"
        })
    );

    EXPECT_EQ(result, args);
}

TEST(Args, MissingValue) {
    auto args = cli_args {
        .file = "/path/to/something",
        .count = 2
    };

    EXPECT_THROWN_MESSAGE(
        nxs::args::parse<cli_args>(
            std::span<const std::string_view>({
                "--file"
            })
        ),
        "Option --file is missing a value"
    );
}

TEST(Args, InvalidType) {
    auto args = cli_args {
        .file = "/path/to/something",
        .count = 2
    };

    EXPECT_THROWN_MESSAGE(
        nxs::args::parse<cli_args>(
            std::span<const std::string_view>({
                "--count", "A"
            })
        ),
        "Failed to parse option --count"
    );
}
