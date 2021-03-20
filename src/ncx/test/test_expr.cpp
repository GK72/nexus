#include <catch2/catch.hpp>

#include "expr.h"

namespace nxs::test {

using namespace nxs::expr;

TEST_CASE("isOperator", "[expr]") {
    using namespace nxs::expr::detail;

    CHECK(isOperator('-'));
    CHECK(isOperator('+'));
    CHECK(isOperator('/'));
    CHECK(isOperator('*'));
    CHECK(isOperator('^'));
}

TEST_CASE("isParen", "[expr]") {
    using namespace nxs::expr::detail;

    CHECK(isParen('('));
    CHECK(isParen(')'));
}

TEST_CASE("isWhitespace", "[expr]") {
    using namespace nxs::expr::detail;

    CHECK(isWhitespace(' '));
    CHECK(isWhitespace('\t'));
    CHECK(isWhitespace('\n'));
}

TEST_CASE("rpn", "[expr]") {
    using namespace nxs::expr::detail;

    CHECK(nxs::expr::toString(rpn("2+3")) == " 2 3 +");
    CHECK(nxs::expr::toString(rpn("2 + 3")) == " 2 3 +");
    CHECK(nxs::expr::toString(rpn("22+3")) == " 22 3 +");
    CHECK(nxs::expr::toString(rpn("22+    31230")) == " 22 31230 +");
}

TEST_CASE("rpn multiple operators", "[expr]") {
    using namespace nxs::expr::detail;

    CHECK(nxs::expr::toString(rpn("2*3+4")) == " 2 3 * 4 +");
    CHECK(nxs::expr::toString(rpn("2+3*4")) == " 2 3 4 * +");

    CHECK(nxs::expr::toString(rpn("2+3*4-6/2")) == " 2 3 4 * 6 2 / - +");
}

TEST_CASE("rpn precedence", "[expr]") {
    using namespace nxs::expr::detail;

    CHECK(nxs::expr::toString(rpn("3^2^2 * 10")) == " 3 2 2 ^ ^ 10 *");
}

TEST_CASE("rpn parens", "[expr]") {
    using namespace nxs::expr::detail;

    CHECK(nxs::expr::toString(rpn("(2+3)*5")) == " 2 3 + 5 *");
    CHECK(nxs::expr::toString(rpn("((2+3)*5)^2")) == " 2 3 + 5 * 2 ^");
}

TEST_CASE("simple expr eval", "[expr]") {
    SECTION("one digit") {
        CHECK(evalExpr("2+3") == 5);
        CHECK(evalExpr("8-2") == 6);
        CHECK(evalExpr("2-6") == -4);
        CHECK(evalExpr("2*4") == 8);
        CHECK(evalExpr("6/2") == 3);
        CHECK(evalExpr("2^3") == 8);
    }

    SECTION("multiple digits and spaces") {
        CHECK(evalExpr("20  +3") == 23);
        CHECK(evalExpr("20  +   36  ") == 56);
    }
}

TEST_CASE("multiple operators expr eval", "[expr]") {
    CHECK(evalExpr("2*3+4") == 10);
    CHECK(evalExpr("2+3*4-6/2") == 11);
    CHECK(evalExpr("3^2^2") == 81);
}

TEST_CASE("parens expr eval", "[expr]") {
    CHECK(evalExpr("((2+3)*5)^2") == 625);
}

TEST_CASE("dbg", "[expr]") {
    CHECK(evalExpr("(3^2^2 * 10)") == 810);
}

TEST_CASE("combined expr eval", "[expr]") {
    CHECK(evalExpr("(4*5^2)") == 100);
    CHECK(evalExpr("(8-4)") == 4);
    CHECK(evalExpr("(3^2^2 * 10)") == 810);
    CHECK(evalExpr("(100 - 3^2^2 * 10)") == -710);

    CHECK(evalExpr("(4*5^2) / (8-4) + (100 - 3^2^2 * 10) / (20-15)") == -117);
                //  -------   -----          ----------    -------
                //    100   /   4   +   100  -   810     /    5
                //     ----------       -----------------------
                //         25       +       -710         /    5
                //                          -------------------
                //         25       +               -142
}

} // namespace nxs::test
