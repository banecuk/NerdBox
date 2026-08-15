#include <algorithm>
#include <cstdint>
#include <set>
#include <vector>

#include "ThreadStaggerScheduler.h"

#include <gtest/gtest.h>

namespace {

constexpr uint8_t kBarCount = 28;
constexpr float kFraction = 0.5f;
constexpr uint32_t kFallback = 600;
constexpr uint32_t kMinPeriod = 100;
constexpr uint32_t kMaxPeriod = 3000;

ThreadStaggerScheduler makeScheduler(uint8_t barCount = kBarCount, uint32_t seedValue = 42) {
    ThreadStaggerScheduler s;
    s.configure(barCount, kFraction, kFallback, kMinPeriod, kMaxPeriod);
    s.seed(seedValue);
    return s;
}

// Finds each bar's release offset (relative to epochStart) by linear probing
// — cheap for the small windows/bar counts under test, and doesn't require
// exposing internal rank/release-time state.
std::vector<uint32_t> releaseOffsets(const ThreadStaggerScheduler& s, uint8_t barCount,
                                     uint32_t epochStart, uint32_t window) {
    std::vector<uint32_t> offsets(barCount, window + 1);
    for (uint8_t i = 0; i < barCount; ++i) {
        for (uint32_t offset = 0; offset <= window; ++offset) {
            if (s.isReleased(i, epochStart + offset)) {
                offsets[i] = offset;
                break;
            }
        }
    }
    return offsets;
}

}  // namespace

TEST(ThreadStaggerSchedulerTest, WindowBoundIsStrictlyInsideHalfPeriod) {
    auto s = makeScheduler();
    s.tick(1000, 1000);  // first epoch: period unknown, uses fallback (600ms) -> window 300ms

    const uint32_t epochStart = 1000;
    const uint32_t window = s.windowMs();
    EXPECT_EQ(window, kFallback / 2);

    const auto offsets = releaseOffsets(s, kBarCount, epochStart, window);
    const uint32_t maxOffset = *std::max_element(offsets.begin(), offsets.end());
    const uint32_t expectedMaxOffset = window * (kBarCount - 1) / kBarCount;

    EXPECT_LT(maxOffset, window);
    EXPECT_EQ(maxOffset, expectedMaxOffset);
}

TEST(ThreadStaggerSchedulerTest, AllReleasedByEndOfWindow) {
    auto s = makeScheduler();
    s.tick(1000, 1000);
    const uint32_t releaseByMs = 1000 + s.windowMs();

    for (uint8_t i = 0; i < kBarCount; ++i) {
        EXPECT_TRUE(s.isReleased(i, releaseByMs)) << "bar " << (int)i << " not released";
    }
}

TEST(ThreadStaggerSchedulerTest, OnlyRankZeroReleasedAtEpochStart) {
    auto s = makeScheduler();
    s.tick(1000, 1000);

    int releasedCount = 0;
    for (uint8_t i = 0; i < kBarCount; ++i) {
        if (s.isReleased(i, 1000)) {
            ++releasedCount;
        }
    }
    EXPECT_EQ(releasedCount, 1);
}

TEST(ThreadStaggerSchedulerTest, PeriodMeasurementConverges) {
    auto s = makeScheduler();
    s.tick(0, 1);  // first epoch, period unknown
    EXPECT_EQ(s.periodMs(), kFallback);

    s.tick(600, 2);  // second epoch, delta = 600
    EXPECT_EQ(s.periodMs(), 600u);
    EXPECT_EQ(s.windowMs(), 300u);

    s.tick(1200, 3);  // third epoch, delta = 600 again
    EXPECT_EQ(s.periodMs(), 600u);
    EXPECT_EQ(s.windowMs(), 300u);
}

TEST(ThreadStaggerSchedulerTest, PeriodClampsAtBothEnds) {
    auto s = makeScheduler();
    s.tick(0, 1);
    s.tick(10, 2);  // delta = 10, clamped to kMinPeriod (100)
    EXPECT_GE(s.periodMs(), kMinPeriod);

    auto s2 = makeScheduler();
    s2.tick(0, 1);
    s2.tick(60000, 2);  // delta = 60000, clamped to kMaxPeriod (3000)
    EXPECT_LE(s2.periodMs(), kMaxPeriod);
}

TEST(ThreadStaggerSchedulerTest, FallbackPeriodBeforeSecondEpoch) {
    auto s = makeScheduler();
    s.tick(0, 1);
    EXPECT_EQ(s.periodMs(), kFallback);
}

TEST(ThreadStaggerSchedulerTest, PatternVarietyNeverRepeatsConsecutively) {
    auto s = makeScheduler(kBarCount, 12345);
    std::set<uint8_t> seenPatterns;
    uint8_t lastPattern = 255;
    uint32_t stamp = 1;
    uint32_t now = 0;

    for (int i = 0; i < 50; ++i) {
        now += 600;
        ++stamp;
        s.tick(now, stamp);
        const uint8_t pattern = s.lastPattern();
        if (i > 0) {
            EXPECT_NE(pattern, lastPattern);
        }
        lastPattern = pattern;
        seenPatterns.insert(pattern);
    }

    EXPECT_GE(seenPatterns.size(), 4u);
}

TEST(ThreadStaggerSchedulerTest, PermutationValidityForVariousBarCounts) {
    for (uint8_t n : {1, 2, 7, 8, 28}) {
        auto s = makeScheduler(n, 777);
        uint32_t stamp = 1;
        uint32_t now = 0;
        for (int epoch = 0; epoch < 10; ++epoch) {
            now += 600;
            ++stamp;
            s.tick(now, stamp);

            const uint32_t window = s.windowMs();
            const auto offsets = releaseOffsets(s, n, now, window);

            // Ranks are a permutation of [0, n): each bar's release offset is
            // rank*window/n for a distinct rank in [0, n).
            std::set<uint32_t> distinctOffsets(offsets.begin(), offsets.end());
            EXPECT_EQ(distinctOffsets.size(), static_cast<size_t>(n))
                << "n=" << (int)n << " epoch=" << epoch;
        }
    }
}

TEST(ThreadStaggerSchedulerTest, RolloverSafeAcrossMillisWrap) {
    auto s = makeScheduler();
    const uint32_t epochStart = 0xFFFFF000u;
    s.tick(epochStart, 1);

    const uint32_t window = s.windowMs();
    // now wraps past zero
    const uint32_t nowAfterWrap = epochStart + window;  // wraps around uint32_t
    for (uint8_t i = 0; i < kBarCount; ++i) {
        EXPECT_TRUE(s.isReleased(i, nowAfterWrap));
    }
}

TEST(ThreadStaggerSchedulerTest, ReleaseAllIgnoresPattern) {
    auto s = makeScheduler();
    s.tick(1000, 1000);
    s.releaseAll(1000);

    for (uint8_t i = 0; i < kBarCount; ++i) {
        EXPECT_TRUE(s.isReleased(i, 1000));
    }
}

TEST(ThreadStaggerSchedulerTest, RepeatedTickWithSameStampDoesNotReshuffle) {
    auto s = makeScheduler();
    s.tick(1000, 1000);
    const uint8_t patternAfterFirst = s.lastPattern();
    const uint32_t windowAfterFirst = s.windowMs();

    s.tick(1010, 1000);  // same stamp: not a new epoch
    s.tick(1020, 1000);

    EXPECT_EQ(s.lastPattern(), patternAfterFirst);
    EXPECT_EQ(s.windowMs(), windowAfterFirst);
}
