#include <catch2/catch.hpp>

#include "linkedList.h"
#include <list>

TEST_CASE("iterator", "[linkedList], [data]") {
    nxs::LinkedList<int> list;
    CHECK(list.begin() == std::end(list));
}

TEST_CASE("hook node", "[linkedList], [data]") {
    using namespace nxs::detail;

    NodeBase node1 {};
    NodeBase node2 {};
    NodeBase nodeNew {};

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

TEST_CASE("push", "[linkedList], [data]") {
    nxs::LinkedList<int> list;
    list.push(2);

    CHECK(*list.begin() == 2);
    CHECK(++list.begin() == std::end(list));

    list.push(3);
    auto it = list.begin();
    CHECK(*it == 2);
    ++it;
    CHECK(*it == 3);
}
