// **********************************************
// ** gkpro @ 2020-02-15                       **
// **                                          **
// **           ---  G-Library  ---            **
// **             Datetime Header              **
// **                                          **
// **********************************************

#pragma once
#include <array>

#ifndef DATE_OSIZE
#define DATE_OSIZE
#endif


namespace glib {
namespace date {

class DateImpl_Compact;
class DateImpl_Speed;

#ifdef DATE_OSIZE
using DefDateImpl = DateImpl_Compact;
#elif DATE_OSPEED
using DefDateImpl = DateImpl_Speed;
#endif

using gint = size_t;


static constexpr auto DAYS_IN_MONTHS = std::array{ 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

enum class NAME_MONTHS {
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

enum class NAME_DAYS {
    MONDAY = 1
    ,TUESDAY
    ,WEDNESDAY
    ,THURSDAY
    ,FRIDAY
    ,SATURDAY
    ,SUNDAY
};

bool isDateValid(gint year, gint month, gint day);


class DateImpl_Compact {
public:
    DateImpl_Compact(gint year, gint month, gint day);

    bool isValid()      { return m_rep != 0; }
    gint getYear();
    gint getMonth();
    gint getDay();

    void setYear(gint year);
    void setMonth(gint month);
    void setDay(gint day);

private:
    gint m_rep;

    static gint calculate(gint year, gint month, gint day);
};


class DateImpl_Speed {
public:
    DateImpl_Speed(gint year, gint month, gint day);

private:
    unsigned short m_year;
    unsigned char  m_month;
    unsigned char  m_day;
};


template <class DateImpl = DefDateImpl>
class Date
{
public:
    Date(gint year, gint month, gint day)
        : m_rep(year, month, day) {}

    Date<DateImpl> addYear(int year);
    Date<DateImpl> addMonth(int month);
    Date<DateImpl> addDay(int day);

    bool isValid()               { return m_rep.isValid(); }
    gint getYear()               { return m_rep.getYear(); }
    gint getMonth()              { return m_rep.getMonth(); }
    gint getDay()                { return m_rep.getDay(); }

    void setYear(gint year)      { m_rep.setYear(year); }
    void setMonth(gint month)    { m_rep.setMonth(month); }
    void setDay(gint day)        { m_rep.setDay(day); }

private:
    DateImpl m_rep;
};



} // End of namespace date
} // End of namespace glib