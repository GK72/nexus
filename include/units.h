#pragma once

#include <numeric>
#include <ratio>

namespace glib::units {

template <class Rep, class Period = std::ratio<1>> class Unit;

template <class R1, class R2>
struct _ratioGCD
{
    using type =
        std::ratio<std::gcd(R1::num, R2::num),
                   std::lcm(R1::den, R2::den)>;
};

}

// Common type specialization
namespace std {

template <class Rep1, class Period1, class Rep2, class Period2>
struct common_type<
    glib::units::Unit<Rep1, Period1>,
    glib::units::Unit<Rep2, Period2>>
{
    using type = glib::units::Unit<
        typename common_type<Rep1, Rep2>::type,
        typename glib::units::_ratioGCD<Period1, Period2>::type>;
};

} // namespace std (common type specialization)

namespace glib::units {

// --------------------------------------==[ Casting ]==--------------------------------------------

/**
 * @brief No conversion
 */
template <class FromUnit, class ToUnit,
          class Period = typename std::ratio_divide<
                typename FromUnit::Period,
                typename ToUnit::Period>::type,
          bool = Period::num == 1,
          bool = Period::den == 1>
struct _unit_cast;

/**
 * @brief Representation casting
 */
template <class FromUnit, class ToUnit, class Period>
struct _unit_cast<FromUnit, ToUnit, Period, /* num = 1 */ true, /* den = 1 */ true>
{
    constexpr ToUnit operator()(const FromUnit& x) const {
        return ToUnit(static_cast<typename ToUnit::Rep>(x.count()));
    }
};

/**
 * @brief Representation casting and Period down conversion
 */
template <class FromUnit, class ToUnit, class Period>
struct _unit_cast<FromUnit, ToUnit, Period, /* num = 1 */ true, /* den = 1 */ false>
{
    using CT =
        typename std::common_type<
            typename ToUnit::Rep,
            typename FromUnit::Rep,
            intmax_t
        >::type;
    constexpr ToUnit operator()(const FromUnit& x) const {
        return ToUnit(static_cast<typename ToUnit::Rep>(
            static_cast<CT>(x.count()) / static_cast<CT>(Period::den))
        );
    }
};

/**
 * @brief Representation casting and Period up conversion
 */
template <class FromUnit, class ToUnit, class Period>
struct _unit_cast<FromUnit, ToUnit, Period, /* num = 1 */ false, /* den = 1 */ true>
{
    using CT =
        typename std::common_type<
            typename ToUnit::Rep,
            typename FromUnit::Rep,
            intmax_t
        >::type;
    constexpr ToUnit operator()(const FromUnit& x) const {
        return ToUnit(static_cast<typename ToUnit::Rep>(
            static_cast<CT>(x.count()) * static_cast<CT>(Period::num))
        );
    }
};

/**
 * @brief Representation casting and Period conversion
 */
template <class FromUnit, class ToUnit, class Period>
struct _unit_cast<FromUnit, ToUnit, Period, /* num = 1 */ false, /* den = 1 */ false>
{
    using CT =
        typename std::common_type<
            typename ToUnit::Rep,
            typename FromUnit::Rep,
            intmax_t
        >::type;
    constexpr ToUnit operator()(const FromUnit& x) const {
        return ToUnit(static_cast<typename ToUnit::Rep>(
            static_cast<CT>(x.count())
                * static_cast<CT>(Period::num)
                / static_cast<CT>(Period::den))
        );
    }
};

/**
 * @brief Wrapper for casting
 */
template <class ToUnit, class Rep, class Period>
inline constexpr
ToUnit
unit_cast(const Unit<Rep, Period>& x)
{
    return _unit_cast<Unit<Rep, Period>, ToUnit>()(x);
}

// ------------------------==[ Relational operator implementations ]==------------------------------

template <class Lhs, class Rhs>
struct _unitEq {
    constexpr bool operator()(const Lhs& lhs, const Rhs& rhs) const {
        using CT = typename std::common_type_t<Lhs, Rhs>;
        return CT(lhs).count() == CT(rhs).count();
    }
};

template <class Lhs>
struct _unitEq<Lhs, Lhs> {
    constexpr bool operator()(const Lhs& lhs, const Lhs& rhs) const {
        return lhs.count() == rhs.count();
    }
};

// -------------------------------------==[ UNIT CLASS ]==------------------------------------------

template <class _Rep, class _Period>
class Unit {
    // static_assert(!__is_duration<_Rep>::value, "A duration representation can not be a duration");
    // static_assert(__is_ratio<_Period>::value, "Second template parameter of duration must be a std::ratio");
    // static_assert(_Period::num > 0, "duration period must be positive");
public:
    using Period = typename _Period::type;
    using Rep    = _Rep;

    constexpr Unit() = default;

    template <class Rep2>
    constexpr explicit Unit(const Rep2& value
        // ,typename std::enable_if<
        //     std::is_arithmetic<Rep2>::value>::type* = 0
        )
        : m_value(value)
    {}

    template <class Rep2, class Period2>
    constexpr explicit Unit(const Unit<Rep2, Period2>& value
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

template <class Rep1, class Period1, class Rep2, class Period2>
inline constexpr bool operator==(const Unit<Rep1, Period1>& lhs, const Unit<Rep2, Period2>& rhs) {
    return _unitEq<Unit<Rep1, Period1>, Unit<Rep2, Period2>>()(lhs, rhs);
};



} // namespace glib::units
