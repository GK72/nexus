// **********************************************
// ** gkpro @ 2020-02-15                       **
// **                                          **
// **           ---  G-Library  ---            **
// **         Datatable Implementation         **
// **                                          **
// **********************************************

#include "datetime.h"

namespace glib {
namespace date {


bool isDateValid(gint year, gint month, gint day) {
    if (year <= 0 || month <= 0 || day <= 0)    { return false; }
    if (month == 2
        && year % 4 == 0
        && day == 29
       ) { return true; }
    if (DAYS_IN_MONTHS[month - 1] < day)   { return false; }

    return true;
}

DateImpl_Compact::DateImpl_Compact(gint year, gint month, gint day) {
    m_rep = isDateValid(year, month, day) ? calculate(year, month, day) : 0;
}

gint DateImpl_Compact::calculate(gint year, gint month, gint day) {
    return DAYS_IN_MONTHS[month - 1] * month + day;
}

template <class DateImpl>
Date<DateImpl> Date<DateImpl>::addYear(int year) {
    return Date(getYear() + year, getMonth(), getDay());
}

template <class DateImpl>
Date<DateImpl> Date<DateImpl>::addMonth(int month) {
    return Date(
        getYear() + month / 12
        ,(12 + getMonth() + month) % 12
        ,getDay()
    );
}

template <class DateImpl>
Date<DateImpl> Date<DateImpl>::addDay(int day) {
    return Date(getYear(), getMonth(), getDay() + day);
}



} // End of namespace date
} // End of namespace glib