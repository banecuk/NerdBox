#include <climits>

#include "core/JobScheduler.h"
#include "utils/Cooldown.h"

#include <gtest/gtest.h>

TEST(CooldownTest, InactiveByDefault) {
    Cooldown c;
    EXPECT_FALSE(c.active(0));
    EXPECT_FALSE(c.active(ULONG_MAX));
}

TEST(CooldownTest, ActiveUntilDurationElapses) {
    Cooldown c;
    c.start(1000, 500);
    EXPECT_TRUE(c.active(1000));
    EXPECT_TRUE(c.active(1499));
    EXPECT_FALSE(c.active(1500));
}

TEST(CooldownTest, ClearEndsItEarly) {
    Cooldown c;
    c.start(1000, 500);
    c.clear();
    EXPECT_FALSE(c.active(1001));
}

TEST(CooldownTest, WrapSafeAcrossMillisRollover) {
    Cooldown c;
    c.start(ULONG_MAX - 100, 500);
    EXPECT_TRUE(c.active(ULONG_MAX));
    EXPECT_TRUE(c.active(398));  // 499 ms after start, past the wrap
    EXPECT_FALSE(c.active(399));
}

// The B35 scenario: a backoff that was never started (or long expired) must
// read as "due now" no matter how far millis() has advanced — the old
// JobDue::at(0) went "into the future" once now passed 2^31.
TEST(CooldownTest, AfterCooldownIsDueWhenNeverStarted) {
    Cooldown c;
    const unsigned long lateNow = 0x80000001UL;  // > 2^31 ms after boot
    EXPECT_TRUE(JobScheduler::isDue(JobDue::afterCooldown(c, lateNow), lateNow));
    EXPECT_FALSE(JobScheduler::isDue(JobDue::at(0), lateNow));  // the old bug
}

TEST(CooldownTest, AfterCooldownIsDueLongAfterAnOldFailure) {
    Cooldown c;
    c.start(1000, 2000);
    const unsigned long lateNow = 1000 + 0x80000000UL;
    EXPECT_TRUE(JobScheduler::isDue(JobDue::afterCooldown(c, lateNow), lateNow));
}

TEST(CooldownTest, AfterCooldownWaitsWhileActive) {
    Cooldown c;
    c.start(1000, 2000);
    EXPECT_FALSE(JobScheduler::isDue(JobDue::afterCooldown(c, 1500), 1500));
    EXPECT_TRUE(JobScheduler::isDue(JobDue::afterCooldown(c, 3000), 3000));
}
