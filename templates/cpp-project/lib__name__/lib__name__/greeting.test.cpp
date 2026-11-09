/**
 * Part of @part_of@
 *
 * @brief   Unit tests of the placeholder module.
 *
 * @author      @author@
 * @created     @date@
 * @updated     @date@
 * @reviewed    none
 */

#include <lib@name@/greeting.hpp>

#include <gtest/gtest.h>

TEST(Greeting, GreetsByName) {
    EXPECT_EQ(@namespace@::greet("Nexus"), "Hello, Nexus!");
}

TEST(Greeting, EmptyNameGreetsTheWorld) {
    EXPECT_EQ(@namespace@::greet(""), "Hello, world!");
}
