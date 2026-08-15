#include "CalendarMath.h"

namespace CalendarMath {

bool isLeapYear(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

uint8_t daysInMonth(int year, uint8_t month) {
    static constexpr uint8_t kDays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month == 2 && isLeapYear(year))
        return 29;
    return kDays[month - 1];
}

uint8_t firstWeekdayMonFirst(int year, uint8_t month) {
    // Sakamoto's algorithm for day=1 of `month`; returns 0=Sunday..6=Saturday.
    static constexpr int kT[12] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    int y = year;
    if (month < 3)
        y -= 1;
    const int sundayFirst = (y + y / 4 - y / 100 + y / 400 + kT[month - 1] + 1) % 7;
    return static_cast<uint8_t>((sundayFirst + 6) % 7);
}

void stepMonth(int& year, uint8_t& month, int8_t delta) {
    int total = static_cast<int>(month) - 1 + delta;
    int yearDelta = total / 12;
    int newMonth = total % 12;
    if (newMonth < 0) {
        newMonth += 12;
        yearDelta -= 1;
    }
    year += yearDelta;
    month = static_cast<uint8_t>(newMonth + 1);
}

const char* monthName(uint8_t month) {
    static const char* const kNames[12] = {"JANUARY", "FEBRUARY", "MARCH",     "APRIL",
                                           "MAY",     "JUNE",     "JULY",      "AUGUST",
                                           "SEPTEMBER", "OCTOBER", "NOVEMBER", "DECEMBER"};
    if (month < 1 || month > 12)
        return "";
    return kNames[month - 1];
}

}  // namespace CalendarMath
