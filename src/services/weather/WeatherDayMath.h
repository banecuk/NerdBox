#pragma once

#include <time.h>

// Pure "is the cached forecast a day behind?" arithmetic, extracted from the
// two copy-pasted widget checkMidnightRollover() methods (A11) so WeatherJob
// can own the check and it can be host-tested (test/WeatherDayMathTest.cpp).
// No Arduino dependency; the only platform call is mktime(), whose time zone
// is whatever the device's TZ is set to — the same zone open-meteo's
// daily.time[] (local midnight, unixtime) was requested in.
namespace WeatherDayMath {

// Local midnight (00:00:00) of the calendar day `nowLocal` falls on.
// tm_isdst is reset to -1 so mktime() resolves DST for midnight itself, not
// for the current wall-clock hour (they differ on a DST-transition day).
time_t localMidnight(struct tm nowLocal);

// True when the forecast's first day (days[0].dayStart) isn't today's local
// midnight — i.e. the day rolled over since the fetch and the strip would
// show yesterday as "today". Callers skip this entirely when dayCount == 0.
bool needsRollover(const struct tm& nowLocal, time_t firstDayStart);

}  // namespace WeatherDayMath
