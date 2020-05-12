#pragma once

#include <numeric>
#include <ratio>

namespace glib::units {

template <class Rep, class Measure = std::ratio<1>> class Unit;

template <class R1, class R2>
struct _ratioGCD
{
    using type =
        std::ratio<std::gcd(R1::num, R2::num),
                   std::lcm(R1::den, R2::den)>;
};

}

// -----------------------------==[ Common_Type Specialization ]==----------------------------------
namespace std {

template <class Rep1, class Measure1, class Rep2, class Measure2>
struct common_type<
    glib::units::Unit<Rep1, Measure1>,
    glib::units::Unit<Rep2, Measure2>>
{
    using type = glib::units::Unit<
        typename common_type<Rep1, Rep2>::type,
        typename glib::units::_ratioGCD<Measure1, Measure2>::type>;
};

} // namespace std

namespace glib::units {

// -------------------------------------==[ Type Trait ]==------------------------------------------

template <class T>
struct _isUnit : std::false_type {};

template <class Rep, class Measure>
struct _isUnit<Unit<Rep, Measure>> : std::true_type {};

template <class Rep, class Measure>
struct _isUnit<const Unit<Rep, Measure>> : std::true_type {};

template <class Rep, class Measure>
struct _isUnit<volatile Unit<Rep, Measure>> : std::true_type {};

template <class Rep, class Measure>
struct _isUnit<const volatile Unit<Rep, Measure>> : std::true_type {};

// --------------------------------------==[ Casting ]==--------------------------------------------

/**
 * @brief No conversion
 */
template <class FromUnit, class ToUnit,
          class Measure = typename std::ratio_divide<
                typename FromUnit::Measure,
                typename ToUnit::Measure>::type,
          bool = Measure::num == 1,
          bool = Measure::den == 1>
struct _unit_cast;

/**
 * @brief Representation casting
 */
template <class FromUnit, class ToUnit, class Measure>
struct _unit_cast<FromUnit, ToUnit, Measure, /* num = 1 */ true, /* den = 1 */ true>
{
    constexpr ToUnit operator()(const FromUnit& x) const noexcept {
        return ToUnit(static_cast<typename ToUnit::Rep>(x.count()));
    }
};

/**
 * @brief Representation casting and Measure down conversion
 */
template <class FromUnit, class ToUnit, class Measure>
struct _unit_cast<FromUnit, ToUnit, Measure, /* num = 1 */ true, /* den = 1 */ false>
{
    using CT =
        typename std::common_type_t<
            typename ToUnit::Rep,
            typename FromUnit::Rep,
            intmax_t>;
    constexpr ToUnit operator()(const FromUnit& x) const noexcept {
        return ToUnit(static_cast<typename ToUnit::Rep>(
            static_cast<CT>(x.count()) / static_cast<CT>(Measure::den))
        );
    }
};

/**
 * @brief Representation casting and Measure up conversion
 */
template <class FromUnit, class ToUnit, class Measure>
struct _unit_cast<FromUnit, ToUnit, Measure, /* num = 1 */ false, /* den = 1 */ true>
{
    using CT =
        typename std::common_type_t<
            typename ToUnit::Rep,
            typename FromUnit::Rep,
            intmax_t>;
    constexpr ToUnit operator()(const FromUnit& x) const noexcept {
        return ToUnit(static_cast<typename ToUnit::Rep>(
            static_cast<CT>(x.count()) * static_cast<CT>(Measure::num))
        );
    }
};

/**
 * @brief Representation casting and Measure conversion
 */
template <class FromUnit, class ToUnit, class Measure>
struct _unit_cast<FromUnit, ToUnit, Measure, /* num = 1 */ false, /* den = 1 */ false>
{
    using CT =
        typename std::common_type_t<
            typename ToUnit::Rep,
            typename FromUnit::Rep,
            intmax_t>;;
    constexpr ToUnit operator()(const FromUnit& x) const noexcept {
        return ToUnit(static_cast<typename ToUnit::Rep>(
            static_cast<CT>(x.count())
                * static_cast<CT>(Measure::num)
                / static_cast<CT>(Measure::den))
        );
    }
};

/**
 * @brief Wrapper for casting
 */
template <class ToUnit, class Rep, class Measure>
inline constexpr
ToUnit unit_cast(const Unit<Rep, Measure>& x) noexcept
{
    return _unit_cast<Unit<Rep, Measure>, ToUnit>()(x);
}

// ------------------------==[ Relational operator implementations ]==------------------------------

template <class Lhs, class Rhs>             // ----==[ Different types ]==----
struct _unitEq {
    [[nodiscard]] constexpr
    bool operator()(const Lhs& lhs, const Rhs& rhs) const noexcept {
        using CT = typename std::common_type_t<Lhs, Rhs>;
        return CT(lhs).count() == CT(rhs).count();
    }
};

template <class Lhs, class Rhs>
struct _unitLt {
    [[nodiscard]] constexpr
    bool operator()(const Lhs& lhs, const Rhs& rhs) const noexcept {
        using CT = typename std::common_type_t<Lhs, Rhs>;
        return CT(lhs).count() < CT(rhs).count();
    }
};

template <class Lhs>                        // ----==[ Common type ]==----
struct _unitEq<Lhs, Lhs> {
    [[nodiscard]] constexpr
    bool operator()(const Lhs& lhs, const Lhs& rhs) const noexcept {
        return lhs.count() == rhs.count();
    }
};

template <class Lhs>
struct _unitLt<Lhs, Lhs> {
    [[nodiscard]] constexpr
    bool operator()(const Lhs& lhs, const Lhs& rhs) const noexcept {
        return lhs.count() < rhs.count();
    }
};

// -------------------------------------==[ UNIT CLASS ]==------------------------------------------

template <class _Rep, class _Measure>
class Unit {
    static_assert(!_isUnit<_Rep>::value, "A unit representation can not be a unit");
    // static_assert(__is_ratio<_Measure>::value, "Second template parameter of duration must be a std::ratio");
    // static_assert(_Measure::num > 0, "duration period must be positive");
public:
    using Measure = typename _Measure::type;
    using Rep     = _Rep;

    constexpr Unit() = default;

    template <class Rep2>
    constexpr explicit Unit(const Rep2& value
        // ,typename std::enable_if<
        //     std::is_arithmetic<Rep2>::value>::type* = 0
        )
        : m_value(value)
    {}

    template <class Rep2, class Measure2>
    constexpr explicit Unit(const Unit<Rep2, Measure2>& value
        // ,typename std::enable_if<
        //     std::is_arithmetic<Rep2>::value>::type* = 0
        )
        : m_value(unit_cast<Unit>(value).count())
    {}

    Rep count() const { return m_value; }

private:
    Rep m_value;

};

// --------------------------------==[ Relational operators ]==-------------------------------------

template <class Rep1, class Measure1, class Rep2, class Measure2>
[[nodiscard]] inline constexpr
bool operator==(const Unit<Rep1, Measure1>& lhs, const Unit<Rep2, Measure2>& rhs) noexcept {
    return _unitEq<Unit<Rep1, Measure1>, Unit<Rep2, Measure2>>()(lhs, rhs);
};

template <class Rep1, class Measure1, class Rep2, class Measure2>
[[nodiscard]] inline constexpr
bool operator!=(const Unit<Rep1, Measure1>& lhs, const Unit<Rep2, Measure2>& rhs) noexcept {
    return !(lhs == rhs);
};

template <class Rep1, class Measure1, class Rep2, class Measure2>
[[nodiscard]] inline constexpr
bool operator<(const Unit<Rep1, Measure1>& lhs, const Unit<Rep2, Measure2>& rhs) noexcept {
    return _unitLt<Unit<Rep1, Measure1>, Unit<Rep2, Measure2>>()(lhs, rhs);
};

template <class Rep1, class Measure1, class Rep2, class Measure2>
[[nodiscard]] inline constexpr
bool operator>(const Unit<Rep1, Measure1>& lhs, const Unit<Rep2, Measure2>& rhs) noexcept {
    return rhs < lhs;
};

template <class Rep1, class Measure1, class Rep2, class Measure2>
[[nodiscard]] inline constexpr
bool operator>=(const Unit<Rep1, Measure1>& lhs, const Unit<Rep2, Measure2>& rhs) noexcept {
    return !(lhs < rhs);
};

template <class Rep1, class Measure1, class Rep2, class Measure2>
[[nodiscard]] inline constexpr
bool operator<=(const Unit<Rep1, Measure1>& lhs, const Unit<Rep2, Measure2>& rhs) noexcept {
    return !(lhs > rhs);
};

// --------------------------------==[ Arithmetic operators ]==-------------------------------------

template <class Rep1, class Measure1, class Rep2, class Measure2>
[[nodiscard]] inline constexpr
typename std::common_type_t<Unit<Rep1, Measure1>, Unit<Rep2, Measure2>>
operator+(const Unit<Rep1, Measure1>& lhs, const Unit<Rep2, Measure2>& rhs) noexcept
{
    using CT =
        typename std::common_type_t<
            Unit<Rep1, Measure1>,
            Unit<Rep2, Measure2>>;

    return CT(
        CT(lhs).count() + CT(rhs).count()
    );
}


// ------------------------------------==[ Helper Types ]==-----------------------------------------

using bit   = Unit<long long, std::ratio<1, 8>>;
using byte  = Unit<long long>;
using kByte = Unit<long long, std::ratio<1024>>;
using MByte = Unit<long long, std::ratio<1048576>>;
using GByte = Unit<long long, std::ratio<1073741824>>;
using TByte = Unit<long long, std::ratio<1099511627776>>;

// --------------------------------------==[ Literals ]==-------------------------------------------

namespace literals {
    constexpr bit operator""_bit(unsigned long long x) {
        return bit(static_cast<byte::Rep>(x));
    }

    constexpr byte operator""_byte(unsigned long long x) {
        return byte(static_cast<byte::Rep>(x));
    }

    // TODO ...
}

} // namespace glib::units
