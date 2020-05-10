#pragma once

#include <numeric>
#include <ratio>

namespace glib::units {

template <class Rep, class Period = std::ratio<1>> class Size;

}

// Common type specialization

namespace std {

template <class Rep1, class Period1, class Rep2, class Period2>
struct common_type<
    glib::units::Size<Rep1, Period1>,
    glib::units::Size<Rep2, Period2>>
{
    using type = glib::units::Size<
        typename common_type<Rep1, Rep2>::type,
        typename __ratio_gcd<Period1, Period2>::type>;
        // TODO: replace __ratio_gcd
};

}

namespace glib::units {

// Casting

template <class FromSize, class ToSize,
          class Period = typename std::ratio_divide<
                typename FromSize::Period,
                typename ToSize::Period>::type,
          bool = Period::num == 1,
          bool = Period::den == 1>
struct size_cast__;

template <class FromSize, class ToSize, class Period>
struct size_cast__<FromSize, ToSize, Period, true, true>
{
    constexpr ToSize operator()(const FromSize& x) const {
        return ToSize(static_cast<typename ToSize::Rep>(x.count()));
    }
};

template <class FromSize, class ToSize, class Period>
struct size_cast__<FromSize, ToSize, Period, true, false>
{
    using CT = typename std::common_type<
                    typename ToSize::Rep,
                    typename FromSize::Rep,
                    intmax_t>::type;
    constexpr ToSize operator()(const FromSize& x) const {
        return ToSize(static_cast<typename ToSize::Rep>(
            static_cast<CT>(x.count()) / static_cast<CT>(Period::den))
            // TODO: Period::den
        );
    }
};

template <class FromSize, class ToSize, class Period>
struct size_cast__<FromSize, ToSize, Period, false, true>
{
    using CT = typename std::common_type<
                    typename ToSize::Rep,
                    typename FromSize::Rep,
                    intmax_t>::type;
    constexpr ToSize operator()(const FromSize& x) const {
        return ToSize(static_cast<typename ToSize::Rep>(
            static_cast<CT>(x.count()) * static_cast<CT>(Period::num))
        );
    }
};

template <class FromSize, class ToSize, class Period>
struct size_cast__<FromSize, ToSize, Period, false, false>
{
    using CT = typename std::common_type<
                    typename ToSize::Rep,
                    typename FromSize::Rep,
                    intmax_t>::type;
    constexpr ToSize operator()(const FromSize& x) const {
        return ToSize(static_cast<typename ToSize::Rep>(
            static_cast<CT>(x.count())
                * static_cast<CT>(Period::num)
                / static_cast<CT>(Period::den))
        );
    }
};


template <class Rep, class Period = std::ratio<1>> class size;

// Casting interface

template <class ToSize, class Rep, class Period>
inline constexpr
ToSize
size_cast(const size<Rep, Period>& x)
{
    return size_cast__<size<Rep, Period>, ToSize>()(x);
}

// ---------------------------------------------

// Equality

template <class Lhs, class Rhs>
struct SizeEq {
    constexpr bool operator()(const Lhs& lhs, const Rhs& rhs) const {
        using CT = typename std::common_type_t<Lhs, Rhs>;
        return CT(lhs).count() == CT(rhs).count();
    }
};

template <class Lhs>
struct SizeEq<Lhs, Lhs> {
    constexpr bool operator()(const Lhs& lhs, const Lhs& rhs) const {
        return lhs.count() == rhs.count();
    }
};

// -----------------

template <class _Rep, class _Period>
class Size {
public:
    using Period = typename _Period::type;
    using Rep    = _Rep;

    constexpr Size() = default;

    template <class Rep2>
    constexpr explicit Size(const Rep2& value) : m_value(value) {}

    template <class Rep2, class Period2>
    constexpr explicit Size(const Size<Rep2, Period2>& value)
        : m_value(size_cast<Size>(value).count())
    {}

    Rep count()     { return m_value; }

private:
    Rep m_value;

};

template <class Rep1, class Period1, class Rep2, class Period2>
inline constexpr bool operator==(const Size<Rep1, Period1>& lhs, const Size<Rep2, Period2>& rhs) {
    return SizeEq<Size<Rep1, Period1>, Size<Rep2, Period2>>()(lhs, rhs);
};

} // namespace glib::units
