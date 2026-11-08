#include <cstring>

#include "core/ScreenRegistry.h"

#include <gtest/gtest.h>

// The PC-metrics jobs fetch only while one of these screens is active (A12).
// Changing the set is a behaviour change — update this test deliberately.
TEST(ScreenRegistryTest, NeedsPcMetricsSetIsMainGameDisks) {
    EXPECT_TRUE(screenNeedsPcMetrics(ScreenName::MAIN));
    EXPECT_TRUE(screenNeedsPcMetrics(ScreenName::GAME));
    EXPECT_TRUE(screenNeedsPcMetrics(ScreenName::DISKS));

    EXPECT_FALSE(screenNeedsPcMetrics(ScreenName::NONE));
    EXPECT_FALSE(screenNeedsPcMetrics(ScreenName::BOOT));
    EXPECT_FALSE(screenNeedsPcMetrics(ScreenName::SETTINGS));
    EXPECT_FALSE(screenNeedsPcMetrics(ScreenName::CPU_CLOCK));
    EXPECT_FALSE(screenNeedsPcMetrics(ScreenName::PROCESSES));
    EXPECT_FALSE(screenNeedsPcMetrics(ScreenName::WEATHER));
    EXPECT_FALSE(screenNeedsPcMetrics(ScreenName::CALENDAR));
    EXPECT_FALSE(screenNeedsPcMetrics(ScreenName::WIFI));

    size_t count = 0;
    for (const auto& d : kScreens) {
        if (d.needsPcMetrics) ++count;
    }
    EXPECT_EQ(count, 3u);
}

TEST(ScreenRegistryTest, HelperIsConstexpr) {
    static_assert(screenNeedsPcMetrics(ScreenName::MAIN), "MAIN needs PC metrics");
    static_assert(!screenNeedsPcMetrics(ScreenName::WIFI), "WIFI doesn't");
    SUCCEED();
}

TEST(ScreenRegistryTest, EveryRowIsUnique) {
    for (size_t i = 0; i < kScreenCount; ++i) {
        for (size_t j = i + 1; j < kScreenCount; ++j) {
            const auto& a = kScreens[i];
            const auto& b = kScreens[j];
            EXPECT_NE(a.screen, b.screen) << "duplicate screen at rows " << i << "/" << j;
            EXPECT_STRNE(a.name, b.name) << "duplicate name at rows " << i << "/" << j;
            if (a.event != EventType::NONE) {
                EXPECT_NE(a.event, b.event) << "duplicate event at rows " << i << "/" << j;
            }
            if (a.route != nullptr && b.route != nullptr) {
                EXPECT_STRNE(a.route, b.route) << "duplicate route at rows " << i << "/" << j;
            }
        }
    }
}

TEST(ScreenRegistryTest, NoneHasNoRow) {
    for (const auto& d : kScreens) {
        EXPECT_NE(d.screen, ScreenName::NONE);
    }
    EXPECT_STREQ(screenName(ScreenName::NONE), "NONE");
    EXPECT_STREQ(screenName(ScreenName::MAIN), "MAIN");
}
