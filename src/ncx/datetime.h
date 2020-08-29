/*
 * gkpro @ 2020-04-25
 *   Nexus Library
 *   Datetime header
 */

#pragma once

#include <array>
#include <chrono>
#include <string_view>

#include "utility.h"

#if !defined(DATETIME_OSPEED) && !defined(DATETIME_OSIZE)
#define DATETIME_OSPEED
#endif

namespace nxs::datetime {

class DateImpl_Compact;
class DateImpl_Speed;
class TimeImpl_Compact;
class TimeImpl_Speed;

#ifdef DATETIME_OSIZE
using DefDateImpl = DateImpl_Compact;
using DefTimeImpl = TimeImpl_Compact;
#elif defined(DATETIME_OSPEED)
using DefDateImpl = DateImpl_Speed;
using DefTimeImpl = TimeImpl_Speed;
#endif


constexpr auto days_in_4_years = 365 * 4 + 1;
constexpr auto days_in_months = std::array{ 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

enum class name_months {
    JANUARY = 1
    ,FEBRUARY
    ,MARCH
    ,APRIL
    ,MAY
    ,JUNE
    ,JULY
    ,AUGUST
    ,SEPTEMBER
    ,OCTOBER
    ,NOVEMBER
    ,DECEMBER
};

enum class name_days {
    MONDAY = 1
    ,TUESDAY
    ,WEDNESDAY
    ,THURSDAY
    ,FRIDAY
    ,SATURDAY
    ,SUNDAY
};

bool isDateValid(int year, char month, char day);
std::pair<char, char> splitToMonthAndDays(short days);

template <class Time> Time getCurrentEpoch() {
    return std::chrono::duration_cast<Time>(
        std::chrono::system_clock().now().time_since_epoch());
}

class DateImpl_Compact {
public:
    DateImpl_Compact() = default;
    DateImpl_Compact(int year, char month, char day);

    [[nodiscard]] bool isValid() const      { return m_rep != 0; }

    [[nodiscard]] int  year()   const;
    [[nodiscard]] char month()  const;
    [[nodiscard]] char day()    const;

private:
    size_t m_rep;

    static size_t calculate(int year, char month, char day);
};


class DateImpl_Speed {
public:
    DateImpl_Speed() = default;
    DateImpl_Speed(int year, char month, char day)
        : m_year(year), m_month(month), m_day(day)
    {}

    [[nodiscard]] bool isValid()  const           { return isDateValid(m_year, m_month, m_day); }

    [[nodiscard]] int  year()  const              { return m_year; }
    [[nodiscard]] char month() const              { return m_month; }
    [[nodiscard]] char day()   const              { return m_day; }

private:
    int  m_year;
    char m_month;
    char m_day;
};


template <class DateImpl = DefDateImpl>
class Date
{
public:
    Date() = default;
    Date(int year, char month, char day)
        : m_rep(year, month, day)
    {}

    Date(std::string_view date)
        : Date(
            std::stoi(date.substr(0, 4).data()),
            std::stoi(date.substr(5, 2).data()),
            std::stoi(date.substr(8, 2).data()))
    {}

    bool operator== (const Date<>& date) const {
        return year()  == date.year()
            && month() == date.month()
            && day()   == date.day();
    }

    Date<DateImpl> addYear(int year);
    Date<DateImpl> addMonth(int month);
    Date<DateImpl> addDay(int day);

    [[nodiscard]] bool isValid()  const           { return m_rep.isValid(); }

    [[nodiscard]] int  year()  const              { return m_rep.year(); }
    [[nodiscard]] char month() const              { return m_rep.month(); }
    [[nodiscard]] char day()   const              { return m_rep.day(); }

    std::string toString() const {
        return nxs::joinStr("."
            ,nxs::alignRight(std::to_string(year()) , 4, '0')
            ,nxs::alignRight(std::to_string(month()), 2, '0')
            ,nxs::alignRight(std::to_string(day())  , 2, '0')
        );
    }

private:
    DateImpl m_rep;
};


class TimeImpl_Speed {
public:
    TimeImpl_Speed(char hour, char minute, char second)
        : m_hour(hour), m_minute(minute), m_second(second)
    {}

    [[nodiscard]] bool isValid() const      { return true; }

    [[nodiscard]] char hour()    const      { return m_hour; }
    [[nodiscard]] char minute()  const      { return m_minute; }
    [[nodiscard]] char second()  const      { return m_second; }

private:
    char m_hour;
    char m_minute;
    char m_second;
};

template <class TimeImpl = DefTimeImpl>
class Time
{
public:
    Time(int days, char hour, char minute, char second, bool negative = false)
        : Time(hour, minute, second, negative)
    {
        m_days = days;
    }

    Time(char hour, char minute, char second, bool negative = false)
        : m_rep(hour, minute, second)
    {
        m_negative = negative;
    }

    [[nodiscard]] bool isValid() const     { return m_rep.isValid(); }

    [[nodiscard]] int  days()    const     { return days; }
    [[nodiscard]] char hour()    const     { return m_rep.hour(); }
    [[nodiscard]] char minute()  const     { return m_rep.minute(); }
    [[nodiscard]] char second()  const     { return m_rep.second(); }

    [[nodiscard]] std::string toString() const {
        std::string timeStr = m_negative ? "-" : "";
        timeStr += nxs::joinStr(":"
            ,nxs::alignRight(std::to_string(hour())  , 2, '0')
            ,nxs::alignRight(std::to_string(minute()), 2, '0')
            ,nxs::alignRight(std::to_string(second()), 2, '0')
        );

        return m_days == 0
             ? timeStr
             : nxs::joinStr("", m_days, "d ", timeStr);
    }

private:
    TimeImpl m_rep;
    int m_days = 0;
    bool m_negative;
};


template <class DateRep>
[[nodiscard]]
Date<> epochToDate(DateRep epoch)
{
    auto days       = epoch / 86400;
    auto leapyears  = days / days_in_4_years;
    auto remyears   = days % days_in_4_years / 365;
    auto years      = leapyears * 4 + remyears;
    auto nDayInYear = (days - leapyears * days_in_4_years) % 365;

    auto year         = 1970 + years;
    auto [month, day] = splitToMonthAndDays(nDayInYear);

    return Date(year, month, day);
}

template <class TimeRep>
[[nodiscard]]
Time<> epochToTime(TimeRep epoch)
{
    auto absEpoch = std::abs(epoch);
    auto days    = absEpoch / 86400;
    auto seconds = absEpoch % 86400;

    auto hour       = seconds / 3600;
    auto minute     = seconds / 60 % 60;
    auto second     = seconds - (hour * 3600) - (minute * 60);

    return Time(days, hour, minute, second, epoch < 0);
}

template <class TimeRep>
[[nodiscard]]
std::string prettyTime(TimeRep epoch) {
    return epochToTime(epoch).toString();
}

template <class DateRep>
[[nodiscard]]
std::string prettyDate(DateRep epoch) {
    return epochToDate(epoch).toString();
}

template <class Impl>
bool isDateValid(const Date<Impl>& date) {
    return isDateValid(
        date.getYear()
        ,date.getMonth()
        ,date.getDay()
    );
}

char weekNum(Date<> date);

template <class DateImpl = DefDateImpl,
          class TimeImpl = DefTimeImpl>
class DateTime
{
public:
    DateTime(const Date<DateImpl>& date, const Time<TimeImpl>& time)
        : m_date(date), m_time(time)
    {}

    template <class Epoch>
    DateTime(Epoch epoch)
        : DateTime(epochToDate(epoch), epochToTime(epoch))
    {}

    [[nodiscard]] bool isValid()  const     { return m_date.isValid()
                                                  && m_time.isValid(); }

    [[nodiscard]] int  year()     const     { return m_date.year(); }
    [[nodiscard]] char month()    const     { return m_date.month(); }
    [[nodiscard]] char day()      const     { return m_date.day(); }
    [[nodiscard]] char hour()     const     { return m_time.hour(); }
    [[nodiscard]] char minute()   const     { return m_time.minute(); }
    [[nodiscard]] char second()   const     { return m_time.second(); }

    [[nodiscard]] std::string toString() const {
        return nxs::joinStr(" - "
            ,m_date.toString()
            ,m_time.toString()
        );
    }

private:
    Date<DateImpl> m_date;
    Time<TimeImpl> m_time;
};


} // namespace nxs::date
