#include <catch2/catch.hpp>

#include "detector.h"

struct S {
    using type = void;
    int member;
    int func() { return 1; }
};

void freeFunc(int) {}

template <class T> using has_type   = typename T::type;
template <class T> using has_no     = typename T::no;
template <class T> using has_member = decltype(T::member);
template <class T> using has_func   = decltype(std::declval<T&>().func());
template <class T> using is_func    = decltype(freeFunc(std::declval<T>()));

TEST_CASE("Detection Idiom", "[compileTime]") {
    SECTION("Detection") {
        static_assert(nxs::is_detected_v<has_type  , S>);
        static_assert(nxs::is_detected_v<has_type  , S>);
        static_assert(nxs::is_detected_v<has_member, S>);
        static_assert(nxs::is_detected_v<has_func  , S>);
        static_assert(nxs::is_detected_v<is_func   , int>);
        CHECK(true);
    }

    SECTION("Detected Type") {
        static_assert(std::is_same_v<nxs::detected_t<has_member, S>, int>);
        static_assert(nxs::is_detected_exact_v<int, has_member, S>);
        CHECK(true);
    }

    SECTION("Detection with default type") {
        nxs::detected_or_t<short, has_no, S> x;
        static_assert(std::is_same_v<short, decltype(x)>);
    }
}
