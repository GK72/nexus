#pragma once

#include "types.h"

#include <cstdint>

namespace nxs {

/**
 * @brief   A minimal implementation of ring buffer
 */
template <class T>
class ring {
public:
    ring(std::size_t size)
        : m_data(size)
    {}

    void push_back(const T& value) {
        m_data[m_idx] = value;

        if (m_idx == size() - 1) {
            m_idx = 0;
        } else {
            ++m_idx;
        }
    }

    [[nodiscard]] constexpr const T* data()      const noexcept { return m_data.data(); }
    [[nodiscard]] constexpr std::size_t size()   const noexcept { return m_data.size(); }
    [[nodiscard]] constexpr std::size_t offset() const noexcept { return m_idx; }
    [[nodiscard]] constexpr const T& back()      const noexcept { return m_idx == 0 ? m_data[size() - 1] : m_data[m_idx - 1]; }

private:
    nxs::vector<T> m_data;
    std::size_t m_idx = 0;
};

} // namespace nxs
