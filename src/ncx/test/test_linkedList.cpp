#include <catch2/catch.hpp>

#include "linkedList.h"
#include <list>

TEST_CASE("iterator", "[linkedList], [data]") {
    nxs::LinkedList<int> list;
    CHECK(list.begin() == std::end(list));
}

TEST_CASE("hook node", "[linkedList], [data]") {
    using namespace nxs::detail;

    ListNodeBase node1 {};
    ListNodeBase node2 {};
    ListNodeBase nodeNew {};

    node1.prev = &node1;
    node1.next = &node2;

    node2.prev = &node1;
    node2.next = &node1;

    hook(&nodeNew, &node2);

    CHECK(node2.prev == &nodeNew);
    CHECK(node2.next == &node1);
    CHECK(nodeNew.prev == &node1);
    CHECK(nodeNew.next == &node2);
}

TEST_CASE("append", "[linkedList], [data]") {
    nxs::LinkedList<int> list;
    list.append(2);

    CHECK(*list.begin() == 2);
    CHECK(++list.begin() == std::end(list));

    list.append(3);
    auto it = list.begin();
    CHECK(*it == 2);
    ++it;
    CHECK(*it == 3);
}

TEST_CASE("for each loop", "[linkedList], [data]") {
    nxs::LinkedList<int> list;
    list.append(2);
    list.append(5);
    list.append(8);

    for (auto& x : list) {
        x *= 2;
    }

    auto it = std::begin(list);
    CHECK(*it++ == 4);
    CHECK(*it++ == 10);
    CHECK(*it++ == 16);
}
