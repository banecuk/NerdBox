#include "CalendarMath.h"

#include <gtest/gtest.h>

// ─── Leap years ─────────────────────────────────────────────────────────────

TEST(CalendarMathTest, DivisibleBy4IsLeap) {
    EXPECT_TRUE(CalendarMath::isLeapYear(2024));
}

TEST(CalendarMathTest, DivisibleBy100ButNot400IsNotLeap) {
    EXPECT_FALSE(CalendarMath::isLeapYear(1900));
}

TEST(CalendarMathTest, DivisibleBy400IsLeap) {
    EXPECT_TRUE(CalendarMath::isLeapYear(2000));
}

TEST(CalendarMathTest, NotDivisibleBy4IsNotLeap) {
    EXPECT_FALSE(CalendarMath::isLeapYear(2023));
}

// ─── Month lengths ──────────────────────────────────────────────────────────

TEST(CalendarMathTest, FebruaryHas29DaysInLeapYear) {
    EXPECT_EQ(CalendarMath::daysInMonth(2024, 2), 29);
}

TEST(CalendarMathTest, FebruaryHas28DaysInNonLeapYear) {
    EXPECT_EQ(CalendarMath::daysInMonth(1900, 2), 28);
    EXPECT_EQ(CalendarMath::daysInMonth(2023, 2), 28);
}

TEST(CalendarMathTest, ThirtyDayMonthsAreCorrect) {
    EXPECT_EQ(CalendarMath::daysInMonth(2024, 4), 30);
    EXPECT_EQ(CalendarMath::daysInMonth(2024, 6), 30);
    EXPECT_EQ(CalendarMath::daysInMonth(2024, 9), 30);
    EXPECT_EQ(CalendarMath::daysInMonth(2024, 11), 30);
}

TEST(CalendarMathTest, ThirtyOneDayMonthsAreCorrect) {
    EXPECT_EQ(CalendarMath::daysInMonth(2024, 1), 31);
    EXPECT_EQ(CalendarMath::daysInMonth(2024, 12), 31);
}

// ─── First weekday (known reference dates) ─────────────────────────────────

TEST(CalendarMathTest, Jan2024FirstDayIsMonday) {
    // 2024-01-01 was a Monday — Monday-first index 0.
    EXPECT_EQ(CalendarMath::firstWeekdayMonFirst(2024, 1), 0);
}

TEST(CalendarMathTest, Jan2000FirstDayIsSaturday) {
    // 2000-01-01 was a Saturday — Monday-first index 5.
    EXPECT_EQ(CalendarMath::firstWeekdayMonFirst(2000, 1), 5);
}

TEST(CalendarMathTest, Jan1900FirstDayIsMonday) {
    // 1900-01-01 was a Monday — Monday-first index 0.
    EXPECT_EQ(CalendarMath::firstWeekdayMonFirst(1900, 1), 0);
}

// ─── Month stepping ─────────────────────────────────────────────────────────

TEST(CalendarMathTest, StepForwardWithinYear) {
    int year = 2024;
    uint8_t month = 6;
    CalendarMath::stepMonth(year, month, 1);
    EXPECT_EQ(year, 2024);
    EXPECT_EQ(month, 7);
}

TEST(CalendarMathTest, StepBackwardWithinYear) {
    int year = 2024;
    uint8_t month = 6;
    CalendarMath::stepMonth(year, month, -1);
    EXPECT_EQ(year, 2024);
    EXPECT_EQ(month, 5);
}

TEST(CalendarMathTest, StepForwardAcrossYearBoundary) {
    int year = 2024;
    uint8_t month = 12;
    CalendarMath::stepMonth(year, month, 1);
    EXPECT_EQ(year, 2025);
    EXPECT_EQ(month, 1);
}

TEST(CalendarMathTest, StepBackwardAcrossYearBoundary) {
    int year = 2025;
    uint8_t month = 1;
    CalendarMath::stepMonth(year, month, -1);
    EXPECT_EQ(year, 2024);
    EXPECT_EQ(month, 12);
}

TEST(CalendarMathTest, StepByMultipleMonthsAcrossYearBoundary) {
    int year = 2024;
    uint8_t month = 11;
    CalendarMath::stepMonth(year, month, 3);
    EXPECT_EQ(year, 2025);
    EXPECT_EQ(month, 2);
}

// ─── Month names ────────────────────────────────────────────────────────────

TEST(CalendarMathTest, MonthNamesAreCorrect) {
    EXPECT_STREQ(CalendarMath::monthName(1), "JANUARY");
    EXPECT_STREQ(CalendarMath::monthName(8), "AUGUST");
    EXPECT_STREQ(CalendarMath::monthName(12), "DECEMBER");
}
