/*
 * gkpro @ 2021-03-08
 *   Nexus Library
 *   Allocator header
 */

#pragma once

#include <cstddef>
#include <memory>

namespace nxs {
namespace detail {

} // namespace detail

template <class T>
struct AllocatorHeap {
    [[nodiscard]]
    T* allocate(std::size_t n = 1) {
        return static_cast<T*>(::operator new(n * sizeof(T)));
    }

    void deallocate(T* ptr) noexcept { ::operator delete(ptr); }

    template <class... Args>
    void construct(T* ptr, Args&& ...args) {
        ::new(ptr) T(std::forward<Args>(args)...);
    }

    void destroy(T* ptr) noexcept { ptr->~T(); }
};

template <class T, std::size_t S>
class AllocatorBuf {
public:
    [[nodiscard]]
    T* allocate(std::size_t n = 1) {
        if (m_it >= S) { throw "BADALLOC"; }
        auto tmp = m_it;
        m_it += n * sizeof(T);
        return reinterpret_cast<T*>(&m_buf[tmp]);
    }

    /**
     * @brief   Deallocate is a noop
     */
    void deallocate(T* ptr) noexcept {}

    template <class... Args>
    void construct(T* ptr, Args&& ...args) {
        ::new(ptr) T(std::forward<Args>(args)...);
    }

    void destroy(T* ptr) noexcept { ptr->~T(); }

private:
    std::array<std::byte, S> m_buf;
    std::size_t m_it = 0;
};

template <class T> using Allocator = AllocatorHeap<T>;

} // namespace nxs
