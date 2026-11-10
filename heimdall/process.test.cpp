/**
 * Part of Nexus Library
 *
 * @brief   Tests of running a program and capturing its output.
 *
 * @author      Gábor Krisztián Girhiny
 * @created     2026-11-10
 * @updated     2026-11-10
 * @reviewed    none
 * SPDX-License-Identifier: Unlicense
 */

#include "heimdall/process.hpp"

#include <gtest/gtest.h>

TEST(Process, CapturesOutputAndErrorApart) {
    const auto result = heimdall::run_process({ "sh", "-c", "printf 'a\\nb'; printf err >&2; exit 3" });
    EXPECT_EQ(result.out, "a\nb");
    EXPECT_EQ(result.err, "err");
    EXPECT_EQ(result.status, 3);
    EXPECT_FALSE(result.success());
}

TEST(Process, EnvironmentAndDirectory) {
    const auto result = heimdall::run_process({ "sh", "-c", "printf '%s %s' \"$HEIMDALL_TEST\" \"$PWD\"" }, {
        .directory = "/",
        .env = { { "HEIMDALL_TEST", "yes" } },
    });
    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.out, "yes /");
}

TEST(Process, StandardInputIsEmpty) {
    const auto result = heimdall::run_process({ "cat" });
    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.out, "");
}

TEST(Process, TimeoutKillsTheSession) {
    const auto result = heimdall::run_process({ "sh", "-c", "sleep 10 & sleep 10" }, {
        .timeout = std::chrono::milliseconds(200),
        .new_session = true,
    });
    EXPECT_TRUE(result.timed_out);
    EXPECT_FALSE(result.success());
}

TEST(Process, MissingProgram) {
    const auto result = heimdall::run_process({ "heimdall-no-such-program" });
    EXPECT_EQ(result.status, 127);
    EXPECT_FALSE(result.err.empty());
}
