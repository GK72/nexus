#include <catch2/catch.hpp>

#include <utility>

#include "allocator.h"

TEST_CASE("buffer allocator - noexcept specifications", "[alloc]") {
    using Alloc = nxs::AllocatorBuf<int, 1>;
    static_assert(noexcept(std::declval<Alloc>().~Alloc()));
    static_assert(noexcept(std::declval<Alloc>().deallocate(nullptr)));
    static_assert(noexcept(std::declval<Alloc>().destroy(nullptr)));
}

TEST_CASE("buffer allocator - allocate and construct", "[alloc]") {
    constexpr std::size_t BUF = 2 * sizeof(int);
    auto alloc = nxs::AllocatorBuf<int, BUF>();
    auto* ptr = alloc.allocate();
    auto* buf = ptr;

    alloc.construct(ptr, 100);
    ptr = alloc.allocate();
    alloc.construct(ptr, 101);

    CHECK(buf[0] == 100);
    CHECK(buf[1] == 101);
}

bool destructed = false;

struct S {
    int rep;
    S(int x) : rep(x) {}
    ~S() { destructed = true; }
};

TEST_CASE("buffer allocator - destroy", "[alloc]") {
    constexpr std::size_t BUF = 1 * sizeof(int);
    auto alloc = nxs::AllocatorBuf<S, BUF>();

    auto* ptr = alloc.allocate();
    alloc.construct(ptr, 1);
    CHECK(ptr->rep == 1);

    alloc.destroy(ptr);
    CHECK(destructed);
}

TEST_CASE("buffer allocator - overflow", "[alloc]") {
    constexpr std::size_t BUF = 128 * sizeof(int);
    auto alloc = nxs::AllocatorBuf<int, BUF>();

    auto* ptr = alloc.allocate();
    alloc.construct(ptr, 2);

    for (int i = 0; i < 127; ++i) {
        auto* ptr2 = alloc.allocate();
        alloc.construct(ptr2, 2);
    }
    CHECK_THROWS(alloc.allocate());

    CHECK(ptr[127] == 2);
}
