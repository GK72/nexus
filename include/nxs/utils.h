#pragma once

#include <chrono>
#include <exception>
#include <fstream>
#include <filesystem>
#include <memory>
#include <memory_resource>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <spdlog/spdlog.h>
#include <fmt/format.h>

#ifndef NDEBUG
#define IFDBG(x) x
#else
#define IFDBG(x) ((void)0)
#endif

namespace nxs {

namespace detail {
    struct def_parser {
        [[nodiscard]] std::string operator()(std::ifstream&& inf) {
            std::stringstream ss;
            for (std::string line; std::getline(inf, line); ) {
                ss << line << '\n';
            }
            return ss.str();
        }
    };
}

template <class T> struct dependent_false : public std::false_type {};
template <class T> inline constexpr bool dependent_false_v = dependent_false<T>::value;

class unreachable : public std::exception {
public:
    const char* what() const noexcept override {
        return "Unhandled enum";
    }
};

struct identity {
    constexpr auto& operator()(auto& x) noexcept {
        return x;
    }
};

class mem : public std::pmr::memory_resource {
public:
    mem(std::pmr::memory_resource* upstream = std::pmr::get_default_resource())
        : m_upstream(upstream)
    {}

    ~mem() = default;

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

template <class Callable = identity>
class line_parser {
public:
    line_parser(Callable&& callback = {})
        : m_callback(callback)
    {}

    template <class T = std::remove_cvref_t<std::invoke_result_t<Callable, std::string&>>>
    [[nodiscard]] auto operator()(std::ifstream&& inf) {
        auto ret = nxs::vector<T>();
        for (std::string line; std::getline(inf, line); ) {
            ret.push_back(m_callback(line));
        }
        return ret;
    }

private:
    Callable m_callback;
};

[[nodiscard]] inline std::chrono::nanoseconds now() {
    return std::chrono::steady_clock().now().time_since_epoch();
}

[[nodiscard]] inline auto toUs(std::chrono::nanoseconds x) {
    return static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(x).count());
}

template <class Parser = detail::def_parser>
[[nodiscard]] auto readFile(std::string_view path, Parser&& parser = {})
        -> expected<std::remove_cvref_t<std::invoke_result_t<Parser, std::ifstream>>>
{
    const auto fs = std::filesystem::path(path);
    if (not std::filesystem::is_regular_file(fs)) {
        return unexpected(fmt::format("{} is not a regular file!", std::filesystem::absolute(fs).string()));
    }

    return parser(std::ifstream(fs));
}

} // namespace nxs

template<>
struct fmt::formatter<nxs::error> {
    template <class ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }

    template <class FormatContext>
    auto format(const nxs::error& x, FormatContext& ctx) {
        return fmt::format_to(ctx.out(), "{}", x.msg);
    }
};
