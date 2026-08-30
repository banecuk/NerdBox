#include "services/roomClimate/RoomClimateMath.h"

#include <gtest/gtest.h>

// ─── toX10 ──────────────────────────────────────────────────────────────────

TEST(RoomClimateMathTest, PositiveValueRoundsToNearestTenth) {
    EXPECT_EQ(RoomClimateMath::toX10(26.55f), 266);
}

TEST(RoomClimateMathTest, ZeroIsZero) {
    EXPECT_EQ(RoomClimateMath::toX10(0.0f), 0);
}

TEST(RoomClimateMathTest, NegativeValueRoundsHalfAwayFromZero) {
    // Truncation (plain int cast of value*10) would give -4, one tenth too
    // warm — this is exactly the bug WeatherService::extractX10 exists to
    // avoid, and RoomClimateWidget's outdoor-adjacent sensor can read
    // negative.
    EXPECT_EQ(RoomClimateMath::toX10(-0.5f), -5);
}

TEST(RoomClimateMathTest, NegativeValueBelowFreezing) {
    EXPECT_EQ(RoomClimateMath::toX10(-10.5f), -105);
}

TEST(RoomClimateMathTest, RoundsDownWithinTenth) {
    EXPECT_EQ(RoomClimateMath::toX10(26.549f), 265);
}

TEST(RoomClimateMathTest, RoundsUpWithinTenth) {
    EXPECT_EQ(RoomClimateMath::toX10(26.551f), 266);
}

// ─── clampHumidity ──────────────────────────────────────────────────────────

TEST(RoomClimateMathTest, HumidityRoundsToNearestWhole) {
    EXPECT_EQ(RoomClimateMath::clampHumidity(45.08f), 45);
    EXPECT_EQ(RoomClimateMath::clampHumidity(45.5f), 46);
}

TEST(RoomClimateMathTest, HumidityClampsAboveHundred) {
    EXPECT_EQ(RoomClimateMath::clampHumidity(104.9f), 100);
}

TEST(RoomClimateMathTest, HumidityClampsBelowZero) {
    EXPECT_EQ(RoomClimateMath::clampHumidity(-3.0f), 0);
}
