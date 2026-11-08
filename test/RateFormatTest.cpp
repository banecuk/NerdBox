#include <limits>

#include "ui/widgets/base/RateFormat.h"

#include <gtest/gtest.h>

TEST(RateFormatTest, KbToWholeMbRoundsHalfUp) {
    EXPECT_EQ(RateFormat::kbToWholeMb(0.0f), 0);
    EXPECT_EQ(RateFormat::kbToWholeMb(511.0f), 0);
    EXPECT_EQ(RateFormat::kbToWholeMb(512.0f), 1);
    EXPECT_EQ(RateFormat::kbToWholeMb(1024.0f), 1);
    EXPECT_EQ(RateFormat::kbToWholeMb(1536.0f), 2);
    EXPECT_EQ(RateFormat::kbToWholeMb(500.0f * 1024.0f), 500);
}

TEST(RateFormatTest, NegativeOrNanReadsAsZero) {
    EXPECT_EQ(RateFormat::kbToWholeMb(-2048.0f), 0);
    EXPECT_EQ(RateFormat::kbToWholeMb(std::numeric_limits<float>::quiet_NaN()), 0);
}

TEST(RateFormatTest, FormatWholeMb) {
    char buf[8];
    RateFormat::formatWholeMb(buf, sizeof(buf), 3.4f * 1024.0f);
    EXPECT_STREQ(buf, "3");
}

TEST(RateFormatTest, SplitOneDecimal) {
    char i[16];
    char d[4];
    RateFormat::splitOneDecimal(12.46f, true, i, sizeof(i), d, sizeof(d));
    EXPECT_STREQ(i, "12");
    EXPECT_STREQ(d, ".5");
    RateFormat::splitOneDecimal(0.0f, true, i, sizeof(i), d, sizeof(d));
    EXPECT_STREQ(i, "0");
    EXPECT_STREQ(d, ".0");
    RateFormat::splitOneDecimal(9.96f, true, i, sizeof(i), d, sizeof(d));
    EXPECT_STREQ(i, "10");
    EXPECT_STREQ(d, ".0");
}

TEST(RateFormatTest, SplitOneDecimalNoData) {
    char i[16];
    char d[4] = "x";
    RateFormat::splitOneDecimal(42.0f, false, i, sizeof(i), d, sizeof(d));
    EXPECT_STREQ(i, "--");
    EXPECT_STREQ(d, "");
}
