#include "services/network/InternetVerdict.h"

#include <gtest/gtest.h>

using Internet = NetworkStatus::Internet;

namespace {
constexpr size_t kSlots = 8;

// Verdict for a slot pattern: '.' never probed, '+' pass, '-' fail.
Internet verdict(const char* pattern) {
    ProbeResult r[kSlots];
    for (size_t i = 0; i < kSlots; ++i) {
        r[i].probed = pattern[i] != '.';
        r[i].ok = pattern[i] == '+';
    }
    return classifyInternet(r, kSlots);
}
}  // namespace

TEST(InternetVerdictTest, NothingProbedIsUnknown) {
    EXPECT_EQ(verdict("........"), Internet::UNKNOWN);
}

TEST(InternetVerdictTest, AllPassIsOk) {
    EXPECT_EQ(verdict("++++++++"), Internet::OK);
}

TEST(InternetVerdictTest, OneFailureIsStillOk) {
    EXPECT_EQ(verdict("+++-++++"), Internet::OK);
}

TEST(InternetVerdictTest, TwoFailuresIsWarning) {
    EXPECT_EQ(verdict("-++-++++"), Internet::WARNING);
}

TEST(InternetVerdictTest, ThreeOrMoreFailuresIsDegraded) {
    EXPECT_EQ(verdict("---+++++"), Internet::DEGRADED);
    EXPECT_EQ(verdict("-------+"), Internet::DEGRADED);
}

TEST(InternetVerdictTest, AllFailedIsDown) {
    EXPECT_EQ(verdict("--------"), Internet::DOWN);
}

// Warm-up: never-probed slots are neither pass nor fail.
TEST(InternetVerdictTest, UnprobedSlotsDontCountAsFailures) {
    EXPECT_EQ(verdict("+......."), Internet::OK);
    EXPECT_EQ(verdict("++-....."), Internet::OK);
}

TEST(InternetVerdictTest, SingleProbedFailureIsDown) {
    // The only evidence so far is a failure: "every probed slot failed".
    EXPECT_EQ(verdict("-......."), Internet::DOWN);
}

TEST(InternetVerdictTest, DownTakesPrecedenceOverWarningCount) {
    // Two probed, both failed: DOWN, not WARNING.
    EXPECT_EQ(verdict("--......"), Internet::DOWN);
}

TEST(InternetVerdictTest, ZeroCountIsUnknown) {
    EXPECT_EQ(classifyInternet(nullptr, 0), Internet::UNKNOWN);
}
