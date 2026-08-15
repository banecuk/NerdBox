#pragma once

#include <cstdint>

// Pure calendar arithmetic — no Arduino/LGFX dependency, so it's host-tested
// under [env:native] (see test/CalendarMathTest.cpp). Weekdays are computed
// via Sakamoto's algorithm rather than mktime/localtime: deterministic, no
// timezone/DST coupling, and usable off-device.
namespace CalendarMath {

bool isLeapYear(int year);

// month is 1..12.
uint8_t daysInMonth(int year, uint8_t month);

// Weekday of the 1st of the given month, Monday-first: 0=Mon .. 6=Sun.
uint8_t firstWeekdayMonFirst(int year, uint8_t month);

// Steps (year, month) by delta months, wrapping the year on over/underflow.
void stepMonth(int& year, uint8_t& month, int8_t delta);

// "JANUARY" .. "DECEMBER". month is 1..12.
const char* monthName(uint8_t month);

}  // namespace CalendarMath
