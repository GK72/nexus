/*
 * gkpro @ 2021-03-07
 *   Nexus Library
 *     List header (reference material)
 *
 *   Fundamental structure of a linked list using an allocator
 */

#pragma once

#include "allocator.h"

namespace nxs {
namespace detail {

struct NodeBase {
    NodeBase* next;
    NodeBase* prev;
};

template <class T>
struct Node : public NodeBase {
    T value;
};

template <class T>
class ListIterator {
public:
    using Self = ListIterator<T>;
    using NodeType = Node<T>;
    using value_type = T;
    using reference = T&;

    ListIterator(NodeBase* x) : m_node(x) {}

    [[nodiscard]] reference operator*() noexcept { return static_cast<NodeType*>(m_node)->value; }
    Self& operator++()    noexcept { m_node = m_node->next; return *this; }
    Self  operator++(int) noexcept {
        Self tmp = *this;
        m_node = tmp.m_node->next;
        return tmp;
    }

    [[nodiscard]] auto node() noexcept { return m_node; }

    template <class U>
    friend bool operator==(ListIterator<U> lhs, ListIterator<U> rhs) noexcept;

private:
    NodeBase* m_node;
};

inline void init(NodeBase& node) noexcept {
    node.next = node.prev = &node;
}

/**
 * @brief Hook the left node before the right node
 */
inline void hook(NodeBase* lhs, NodeBase* rhs) noexcept {
    lhs->next = rhs;
    lhs->prev = rhs->prev;
    rhs->prev->next = lhs;
    rhs->prev = lhs;
}

template <class T>
[[nodiscard]]
bool operator==(ListIterator<T> lhs, ListIterator<T> rhs) noexcept {
    return lhs.m_node == rhs.m_node;
}

} // namespace detail

/**
 * @brief   A doubly linked list
 *
 * Has a sentinel node, of which prev and next pointers refer to itself if
 * there are no other node i.e. the list is empty, else its next always
 * refers to the first node (used by `begin()`)
 */
template <class T,
          class Alloc = Allocator<detail::Node<T>>>
class LinkedList {
public:
    LinkedList()
        : m_node { &m_node, &m_node }
        , m_alloc()
    {}

    ~LinkedList();

    LinkedList(const LinkedList& x)                = delete;
    LinkedList(LinkedList&& x)            noexcept = delete;
    LinkedList& operator=(const LinkedList& x)     = delete;
    LinkedList& operator=(LinkedList&& x) noexcept = delete;

    [[nodiscard]] auto begin() noexcept { return detail::ListIterator<T>(m_node.next); }
    [[nodiscard]] auto end()   noexcept { return detail::ListIterator<T>(&m_node); }

    /**
     * @brief   Append an element to the end of the list
     */
    void append(const T& x) {
        insert(x, end());
    }

    /**
     * @brief   Insert element before the element pointed by the iterator
     */
    void insert(const T&x, detail::ListIterator<T> it) {
        detail::Node<T>* tmp = createNode(x);
        hook(tmp, it.node());
    }

private:
    detail::NodeBase m_node;
    Alloc m_alloc;

    [[nodiscard]] detail::Node<T>* createNode(const T& x);
    void freeNode(detail::ListIterator<T> it) noexcept;
};

template <class T, class Alloc>
[[nodiscard]]
detail::Node<T>* LinkedList<T, Alloc>::createNode(const T& x) {
    auto* ptr = m_alloc.allocate();
    m_alloc.construct(
        ptr,
        detail::Node<T>{ nullptr, nullptr, x }
    );
    return ptr;
}

template <class T, class Alloc>
void LinkedList<T, Alloc>::freeNode(detail::ListIterator<T> it) noexcept {
    m_alloc.destroy(static_cast<detail::Node<T>*>(it.node()));
    m_alloc.deallocate(static_cast<detail::Node<T>*>(it.node()));
}

template <class T, class Alloc>
LinkedList<T, Alloc>::~LinkedList() {
    auto it = begin();
    if (it == end()) {
        return;
    }

    auto prev = begin();

    while (++it != end()) {
        freeNode(prev);
        prev = it;
    }

    freeNode(prev);
}

} // namespace nxs
