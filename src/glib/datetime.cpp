// **********************************************
// ** gkpro @ 2020-02-15                       **
// **                                          **
// **           ---  G-Library  ---            **
// **         Datatable Implementation         **
// **                                          **
// **********************************************

#include "datetime.h"

namespace glib::datetime {


bool isDateValid(int year, char month, char day) {
    if (year <= 0 || month <= 0 || day <= 0)    { return false; }
    if (month == 2
        && year % 4 == 0
        && day == 29
       ) { return true; }
    if (days_in_months[month - 1] < day)   { return false; }

    return true;
}

std::pair<char, char> splitToMonthAndDays(short days) {
    char month = 0;
    while (days > days_in_months[month]) {
        days -= days_in_months[month];
        ++month;
    }
    return std::pair(month + 1, days);
}

DateImpl_Compact::DateImpl_Compact(int year, char month, char day) {
    m_rep = isDateValid(year, month, day) ? calculate(year, month, day) : 0;
}

size_t DateImpl_Compact::calculate(int year, char month, char day) {
    return year * days_in_months[month - 1] * month + day;
}

template <class DateImpl>
Date<DateImpl> Date<DateImpl>::addYear(int year_) {
    return Date(year() + year_, month(), day());
}

template <class DateImpl>
Date<DateImpl> Date<DateImpl>::addMonth(int month_) {
    return Date(
        year() + month_ / 12
        ,(12 + month() + month_) % 12
        ,day()
    );
}

template <class DateImpl>
Date<DateImpl> Date<DateImpl>::addDay(int day_) {
    return Date(year(), month(), day() + day_);
}



} // End of namespace glib::date