#include "DataFreshnessGuard.h"

#include <gtest/gtest.h>

// ─── Fake clock ─────────────────────────────────────────────────────────────
// DataFreshnessGuard's ClockFn is a bare function pointer (matching millis()'s
// signature), so a capturing lambda can't be used — a mutable global stands
// in for "the current fake time" instead, reset at the top of every test.

static unsigned long g_fakeNowMs = 0;
static unsigned long fakeMillis() {
    return g_fakeNowMs;
}

class DataFreshnessGuardTest : public ::testing::Test {
 protected:
    void SetUp() override { g_fakeNowMs = 0; }
};

TEST_F(DataFreshnessGuardTest, StaleWhenNeverPublished) {
    PublishedFlag flag;
    DataFreshnessGuard guard(flag, 5000, &fakeMillis);
    EXPECT_FALSE(guard.isFresh());
}

TEST_F(DataFreshnessGuardTest, FreshImmediatelyAfterPublish) {
    PublishedFlag flag;
    g_fakeNowMs = 1000;
    flag.publish(g_fakeNowMs);

    DataFreshnessGuard guard(flag, 5000, &fakeMillis);
    EXPECT_TRUE(guard.isFresh());
}

TEST_F(DataFreshnessGuardTest, FreshAtExactTimeoutBoundary) {
    PublishedFlag flag;
    flag.publish(0);

    DataFreshnessGuard guard(flag, 5000, &fakeMillis);
    g_fakeNowMs = 5000;
    EXPECT_TRUE(guard.isFresh());  // <= timeout, inclusive
}

TEST_F(DataFreshnessGuardTest, StaleOneMsPastTimeout) {
    PublishedFlag flag;
    flag.publish(0);

    DataFreshnessGuard guard(flag, 5000, &fakeMillis);
    g_fakeNowMs = 5001;
    EXPECT_FALSE(guard.isFresh());
}

TEST_F(DataFreshnessGuardTest, SetTimeoutChangesThreshold) {
    PublishedFlag flag;
    flag.publish(0);

    DataFreshnessGuard guard(flag, 5000, &fakeMillis);
    g_fakeNowMs = 2000;
    EXPECT_TRUE(guard.isFresh());

    guard.setTimeout(1000);
    EXPECT_EQ(guard.getTimeout(), 1000u);
    EXPECT_FALSE(guard.isFresh());
}

// millis()-style clocks wrap around ULONG_MAX; freshness relies on unsigned
// subtraction wrapping the same way so a fetch just before rollover still
// reads as fresh just after it.
TEST_F(DataFreshnessGuardTest, SurvivesClockWraparound) {
    PublishedFlag flag;
    const unsigned long lastUpdate = 0xFFFFFFFFUL - 10;  // 10 ms before wraparound
    flag.publish(lastUpdate);

    DataFreshnessGuard guard(flag, 100, &fakeMillis);
    g_fakeNowMs = 5;  // wrapped around; 16 ms have actually elapsed
    EXPECT_TRUE(guard.isFresh());

    guard.setTimeout(15);
    EXPECT_FALSE(guard.isFresh());
}

TEST_F(DataFreshnessGuardTest, DefaultTimeoutConstantIs5000) {
    EXPECT_EQ(DataFreshnessGuard::kDefaultTimeoutMs, 5000u);
}

// ─── PublishedFlag::ageMs ───────────────────────────────────────────────────

TEST_F(DataFreshnessGuardTest, AgeIsZeroWhenNeverPublished) {
    PublishedFlag flag;
    EXPECT_EQ(flag.ageMs(12345), 0UL);
}

TEST_F(DataFreshnessGuardTest, AgeIsElapsedSincePublish) {
    PublishedFlag flag;
    flag.publish(1000);
    EXPECT_EQ(flag.ageMs(1000), 0UL);
    EXPECT_EQ(flag.ageMs(3500), 2500UL);
}

TEST_F(DataFreshnessGuardTest, AgeIsWrapSafeAcrossMillisRollover) {
    PublishedFlag flag;
    const unsigned long nearWrap = static_cast<unsigned long>(-1) - 99;  // 100 ms before wrap
    flag.publish(nearWrap);
    EXPECT_EQ(flag.ageMs(400), 500UL);  // 100 ms to wrap + 400 ms after it
}
