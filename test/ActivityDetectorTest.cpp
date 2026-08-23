#include "ActivityDetector.h"

#include <climits>

#include <gtest/gtest.h>

namespace {
// Matches AppConfig::internal::MultiWidgetImpl's defaults.
constexpr uint8_t kEnterPct = 70;
constexpr uint8_t kExitPct = 50;
constexpr uint32_t kQuietMs = 30000;
constexpr uint32_t kEnterHoldMs = 6000;
}  // namespace

TEST(ActivityDetectorTest, ShortBurstUnder5sDoesNotEnter) {
    ActivityDetector d(kEnterPct, kExitPct, kQuietMs, kEnterHoldMs);
    d.tick(0, true, 100, 0);
    d.tick(4900, true, 100, 0);
    EXPECT_FALSE(d.isActive());
}

TEST(ActivityDetectorTest, SustainedLoadEntersJustAfterHold) {
    ActivityDetector d(kEnterPct, kExitPct, kQuietMs, kEnterHoldMs);
    d.tick(0, true, 100, 0);
    d.tick(6100, true, 100, 0);
    EXPECT_TRUE(d.isActive());
}

TEST(ActivityDetectorTest, SingleSpikeDoesNotEnter) {
    ActivityDetector d(kEnterPct, kExitPct, kQuietMs, kEnterHoldMs);
    d.tick(0, true, 100, 0);
    d.tick(200, true, 0, 0);
    d.tick(6200, true, 0, 0);
    EXPECT_FALSE(d.isActive());
}

TEST(ActivityDetectorTest, RepeatedShortBurstsNeverEnter) {
    ActivityDetector d(kEnterPct, kExitPct, kQuietMs, kEnterHoldMs);
    uint32_t now = 0;
    for (int i = 0; i < 20; ++i) {
        d.tick(now, true, 100, 0);
        now += 4000;
        d.tick(now, true, 0, 0);  // drop below exitPct_ resets the hold
        now += 1000;
    }
    EXPECT_FALSE(d.isActive());
}

TEST(ActivityDetectorTest, OscillatingLoadEntersAfterHoldDespiteDips) {
    ActivityDetector d(kEnterPct, kExitPct, kQuietMs, kEnterHoldMs);
    uint32_t now = 0;
    // 60/90 alternating every 500ms — dips stay above exitPct_ (50), so the
    // hold clock (started at the first 90) should never reset.
    for (int i = 0; i < 13; ++i) {
        d.tick(now, true, i % 2 == 0 ? 90 : 60, 0);
        now += 500;
    }
    EXPECT_TRUE(d.isActive());
}

TEST(ActivityDetectorTest, ExactlyExitPctNeitherEntersNorResetsHold) {
    ActivityDetector d(kEnterPct, kExitPct, kQuietMs, kEnterHoldMs);
    d.tick(0, true, 90, 0);       // starts the hold
    d.tick(3000, true, 50, 0);    // dip to exactly exitPct_ — must not reset
    d.tick(6100, true, 90, 0);    // hold should still complete from t=0
    EXPECT_TRUE(d.isActive());
}

TEST(ActivityDetectorTest, ExactlyExitPctAloneNeverEntersWithoutReachingEnter) {
    ActivityDetector d(kEnterPct, kExitPct, kQuietMs, kEnterHoldMs);
    uint32_t now = 0;
    for (int i = 0; i < 20; ++i) {
        d.tick(now, true, kExitPct, 0);
        now += 1000;
    }
    EXPECT_FALSE(d.isActive());
}

TEST(ActivityDetectorTest, CpuAloneSatisfiesEnterCondition) {
    ActivityDetector d(kEnterPct, kExitPct, kQuietMs, kEnterHoldMs);
    d.tick(0, true, 100, 0);
    d.tick(6100, true, 100, 0);
    EXPECT_TRUE(d.isActive());
}

TEST(ActivityDetectorTest, GpuAloneSatisfiesEnterCondition) {
    ActivityDetector d(kEnterPct, kExitPct, kQuietMs, kEnterHoldMs);
    d.tick(0, true, 0, 100);
    d.tick(6100, true, 0, 100);
    EXPECT_TRUE(d.isActive());
}

TEST(ActivityDetectorTest, ExitRequiresFullQuietWindow) {
    ActivityDetector d(kEnterPct, kExitPct, kQuietMs, kEnterHoldMs);
    d.tick(0, true, 100, 0);
    d.tick(6100, true, 100, 0);
    ASSERT_TRUE(d.isActive());

    d.tick(6200, true, 0, 0);
    d.tick(6200 + kQuietMs - 1, true, 0, 0);
    EXPECT_TRUE(d.isActive());  // not quite 30s yet

    d.tick(6200 + kQuietMs, true, 0, 0);
    EXPECT_FALSE(d.isActive());
}

TEST(ActivityDetectorTest, SampleAtOrAboveExitResetsQuietTimer) {
    ActivityDetector d(kEnterPct, kExitPct, kQuietMs, kEnterHoldMs);
    d.tick(0, true, 100, 0);
    d.tick(6100, true, 100, 0);
    ASSERT_TRUE(d.isActive());

    d.tick(10000, true, 0, 0);                 // quiet timer starts
    d.tick(10000 + kQuietMs - 1, true, 60, 0);  // resets it just before expiry
    d.tick(10000 + kQuietMs, true, 0, 0);       // not enough time since reset
    EXPECT_TRUE(d.isActive());
}

TEST(ActivityDetectorTest, StaleDataNeverEnters) {
    ActivityDetector d(kEnterPct, kExitPct, kQuietMs, kEnterHoldMs);
    d.tick(0, false, 100, 100);
    d.tick(6100, false, 100, 100);
    EXPECT_FALSE(d.isActive());
}

TEST(ActivityDetectorTest, StaleDataDoesNotStallQuietTimer) {
    ActivityDetector d(kEnterPct, kExitPct, kQuietMs, kEnterHoldMs);
    d.tick(0, true, 100, 0);
    d.tick(6100, true, 100, 0);
    ASSERT_TRUE(d.isActive());

    d.tick(6200, false, 100, 100);  // fresh == false treated as below-threshold
    d.tick(6200 + kQuietMs, false, 100, 100);
    EXPECT_FALSE(d.isActive());
}

TEST(ActivityDetectorTest, EnterHoldSurvivesMillisRollover) {
    ActivityDetector d(kEnterPct, kExitPct, kQuietMs, kEnterHoldMs);
    const uint32_t start = ULONG_MAX - 3000;
    d.tick(start, true, 100, 0);
    d.tick(start + kEnterHoldMs + 100, true, 100, 0);  // wraps past ULONG_MAX
    EXPECT_TRUE(d.isActive());
}

TEST(ActivityDetectorTest, QuietWindowSurvivesMillisRollover) {
    ActivityDetector d(kEnterPct, kExitPct, kQuietMs, kEnterHoldMs);
    d.tick(0, true, 100, 0);
    d.tick(6100, true, 100, 0);
    ASSERT_TRUE(d.isActive());

    const uint32_t start = ULONG_MAX - 3000;
    d.tick(start, true, 0, 0);
    d.tick(start + kQuietMs + 100, true, 0, 0);  // wraps past ULONG_MAX
    EXPECT_FALSE(d.isActive());
}
