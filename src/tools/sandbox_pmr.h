#pragma once

#include <array>
#include <cstddef>
#include <memory_resource>
#include <vector>

#include "utility.h"

namespace nxs::sandbox {

template <class T>
void printHex(T x, int width = 0) {
    std::cout << std::hex << std::setw(width) << std::setfill('0') << x << ' ';
}

template <class It>
void printHexLine(It begin, It end, size_t line, const std::byte* original) {
    auto offset = line * 16;                            // NOLINT

    std::cout << original + offset << ' ';              // NOLINT
    printHex(offset, 5);                                // NOLINT
    nxs::printnl(":  ");

    std::for_each(begin, end,
        [i=0](auto x) mutable {
            printHex(std::to_integer<int>(x), 2);
            if (++i % 8 == 0) {                         // NOLINT
                nxs::printnl(" ");
            }
        }
    );

    nxs::print();
}

template <class Buffer>
void printBuf(const Buffer& buf) {
    size_t i = 0;
    auto it = std::begin(buf);
    auto itEnd = std::end(buf);

    while (it != std::end(buf)) {
        auto itEnd = std::next(it, 16);                 // NOLINT
        printHexLine(it, itEnd, i++, buf.data());
        it = itEnd;
    }
}

template <class Buffer, class Container>
void print(const Buffer& buf, const Container& cont) {
    nxs::print(" ", "Buffer address   :", static_cast<const void*>(&buf[0]));
    nxs::print(" ", "Container address:", static_cast<const void*>(&cont));
    nxs::print();
    printBuf(buf);
}


class Alloc: public std::pmr::memory_resource {
public:
    Alloc(std::byte* buffer, size_t size)
        : m_buffer(buffer)
        , m_bufsize(size)
        , m_lastaddr(buffer)
    {}

private:
    std::byte* m_buffer;
    size_t m_bufsize;

    std::byte* m_lastaddr;

    void* do_allocate(size_t bytes, [[maybe_unused]] size_t alignment) override {
        debug(bytes);
        if (m_lastaddr + bytes > m_buffer + m_bufsize) {        // NOLINT
            throw std::runtime_error("Allocation failed");
            // return std::pmr::new_delete_resource()->allocate(bytes, alignment);
        }

        void* addr = m_lastaddr;
        printHex(addr);
        nxs::print();

        m_lastaddr += bytes;        // NOLINT
        return addr;
    }

    void debug(size_t bytes) {
        ptrdiff_t used = m_lastaddr - m_buffer;
        nxs::printnl("", "Allocating... ", bytes, " bytes (", used + bytes, " out of ", m_bufsize, ") ");
    }

    void do_deallocate([[maybe_unused]] void* ptr,
                       [[maybe_unused]] size_t bytes,
                       [[maybe_unused]] size_t alignment) override
    {
        nxs::print("No deallocating...");
        // if (fallbackToGlobalNew) {
        //     return std::pmr::new_delete_resource()->deallocate(ptr, bytes, alignment);
        // }
    }

    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
        return this == &other;
    }

};

class MemRes {
public:
    MemRes(size_t size)
        : m_buffer(size)
        , m_alloc(m_buffer.data(), m_buffer.size())
    {
        std::pmr::set_default_resource(&m_alloc);
    }

    std::pmr::memory_resource* resource() { return &m_alloc; }
    const auto& buffer() const            { return m_buffer; }

private:
    std::vector<std::byte> m_buffer;
    Alloc m_alloc;
};


inline void pmr() {
    constexpr auto BUF_SIZE = 32;
    MemRes mem(BUF_SIZE);

    auto vec = std::pmr::vector<int>(mem.resource());
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);
    vec.push_back(4);
    vec.push_back(4);
    vec.push_back(4);

    print(mem.buffer(), vec);
}

} // namespace nxs::sandbox
