#include "services/weather/WeatherDayMath.h"

#include <gtest/gtest.h>

// Both sides of every comparison go through mktime(), so these tests hold in
// whatever TZ the host runs in — they check the day arithmetic, not a zone.

namespace {

struct tm makeLocal(int year, int month, int day, int hour, int min, int sec) {
    struct tm t = {};
    t.tm_year = year - 1900;
    t.tm_mon = month - 1;
    t.tm_mday = day;
    t.tm_hour = hour;
    t.tm_min = min;
    t.tm_sec = sec;
    t.tm_isdst = -1;
    return t;
}

time_t midnightOf(int year, int month, int day) {
    struct tm t = makeLocal(year, month, day, 0, 0, 0);
    return mktime(&t);
}

}  // namespace

TEST(WeatherDayMathTest, LocalMidnightZeroesTimeOfDay) {
    EXPECT_EQ(WeatherDayMath::localMidnight(makeLocal(2026, 11, 8, 13, 45, 12)),
              midnightOf(2026, 11, 8));
}

TEST(WeatherDayMathTest, NoRolloverWhenFirstDayIsToday) {
    EXPECT_FALSE(
        WeatherDayMath::needsRollover(makeLocal(2026, 11, 8, 13, 45, 0), midnightOf(2026, 11, 8)));
}

TEST(WeatherDayMathTest, NoRolloverAtExactMidnight) {
    EXPECT_FALSE(
        WeatherDayMath::needsRollover(makeLocal(2026, 11, 8, 0, 0, 0), midnightOf(2026, 11, 8)));
}

TEST(WeatherDayMathTest, NoRolloverOneSecondBeforeMidnight) {
    EXPECT_FALSE(
        WeatherDayMath::needsRollover(makeLocal(2026, 11, 8, 23, 59, 59), midnightOf(2026, 11, 8)));
}

TEST(WeatherDayMathTest, RolloverWhenForecastIsFromYesterday) {
    EXPECT_TRUE(
        WeatherDayMath::needsRollover(makeLocal(2026, 11, 9, 0, 0, 1), midnightOf(2026, 11, 8)));
}

TEST(WeatherDayMathTest, RolloverAcrossMonthAndYearBoundary) {
    EXPECT_TRUE(
        WeatherDayMath::needsRollover(makeLocal(2027, 1, 1, 8, 0, 0), midnightOf(2026, 12, 31)));
    EXPECT_FALSE(
        WeatherDayMath::needsRollover(makeLocal(2027, 1, 1, 8, 0, 0), midnightOf(2027, 1, 1)));
}

TEST(WeatherDayMathTest, RolloverWhenForecastIsAhead) {
    // A clock that jumped backwards (or a forecast fetched in a different
    // zone) also refetches rather than showing the wrong "today".
    EXPECT_TRUE(
        WeatherDayMath::needsRollover(makeLocal(2026, 11, 8, 12, 0, 0), midnightOf(2026, 11, 9)));
}
