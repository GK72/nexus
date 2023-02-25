#pragma once

#include <coroutine>
#include <exception>
#include <utility>

namespace nxs {

template <class T>
class generator {
public:
    struct promise_type;
    using handle_type = std::coroutine_handle<promise_type>;

    struct promise_type {
        T value;
        std::exception_ptr exception;

        generator<T> get_return_object() {
            return generator(handle_type::from_promise(*this));
        }

        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend()   noexcept { return {}; }
        void return_void() {}

        void unhandled_exception() {
            exception = std::current_exception();
        }

        std::suspend_always yield_value(std::convertible_to<T> auto&& from) {
            value = std::forward<decltype(from)>(from);
            return {};
        }
    };

public:
    generator(handle_type handle)
        : m_handle(handle)
    {}

    ~generator() noexcept {
        m_handle.destroy();
    }

    /**
     * @brief   Whether or not the coroutine is finished
     *
     * The actual value is dependent on the calculation, therefore
     * calling the coroutine and caching the result in the promise.
     */
    explicit operator bool() {
        fill();
        return not m_handle.done();
    }

    /**
     * @brief   Direct call to the coroutine
     *
     * Clears out the cache (flag)
     */
    T operator()() {
        fill();
        m_cached = false;
        return std::move(m_handle.promise().value);
    }

private:
    handle_type m_handle;
    bool m_cached = false;

    /**
     * @brief   Carry out the generation
     */
    void fill() {
        if (not m_cached) {
            m_handle();
            if (m_handle.promise().exception) {
                std::rethrow_exception(m_handle.promise().exception);
            }

            m_cached = true;
        }
    }
};

} // namespace nxs
