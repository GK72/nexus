#pragma once

#include <cstdint>
#include <memory>
#include <memory_resource>

namespace nxs {

class mem : public std::pmr::memory_resource {
public:
    mem(std::pmr::memory_resource* upstream = std::pmr::get_default_resource())
        : m_upstream(upstream)
    {}

    ~mem() override = default;

    #ifndef NDEBUG
    [[nodiscard]] std::size_t totalAllocations()     const noexcept { return m_totalAllocations; }
    [[nodiscard]] std::size_t totalAlloctedBytes()   const noexcept { return m_totalAlloctedBytes; }
    #endif

private:
    std::pmr::memory_resource* m_upstream;

    #ifndef NDEBUG
    std::size_t m_totalAlloctedBytes = 0;
    std::size_t m_totalAllocations= 0;
    #endif

    [[nodiscard]] void* do_allocate(std::size_t bytes, std::size_t alignment) override {
        void* ptr = m_upstream->allocate(bytes, alignment);

        #ifndef NDEBUG
        ++m_totalAllocations;
        m_totalAlloctedBytes += bytes;
        #endif
        return ptr;
    }

    void do_deallocate(void* ptr, std::size_t bytes, std::size_t alignment) noexcept override {
        m_upstream->deallocate(ptr, bytes, alignment);

        #ifndef NDEBUG
        m_totalAlloctedBytes -= bytes;
        #endif
    }

    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
        return m_upstream->is_equal(other);
    }
};

} // namespace nxs
