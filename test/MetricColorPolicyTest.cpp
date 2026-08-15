#include "MetricColorPolicy.h"

#include <gtest/gtest.h>

TEST(MetricColorPolicyTest, NormalRampBelowLowerThresholdIsZero) {
    EXPECT_EQ(MetricColorPolicy::normalizedPercent(10, 50.0f, 90.0f, false), 0);
}

TEST(MetricColorPolicyTest, NormalRampAboveUpperThresholdIsMax) {
    EXPECT_EQ(MetricColorPolicy::normalizedPercent(95, 50.0f, 90.0f, false), 100);
}

TEST(MetricColorPolicyTest, NormalRampMidpointIsHalfway) {
    EXPECT_EQ(MetricColorPolicy::normalizedPercent(70, 50.0f, 90.0f, false), 50);
}

TEST(MetricColorPolicyTest, ReverseRampBelowLowerThresholdIsMax) {
    // e.g. free disk space: low value is bad.
    EXPECT_EQ(MetricColorPolicy::normalizedPercent(10, 50.0f, 90.0f, true), 100);
}

TEST(MetricColorPolicyTest, ReverseRampAboveUpperThresholdIsZero) {
    EXPECT_EQ(MetricColorPolicy::normalizedPercent(95, 50.0f, 90.0f, true), 0);
}

TEST(MetricColorPolicyTest, ReverseRampMidpointIsHalfway) {
    EXPECT_EQ(MetricColorPolicy::normalizedPercent(70, 50.0f, 90.0f, true), 50);
}

// With equal thresholds a value above them is caught by the ">= upperThreshold"
// early-out before the range<=0 guard is ever reached — that guard only
// exists for the strictly-between branch, which is unreachable when
// lowerThreshold <= upperThreshold (an invariant the caller, MetricWidget's
// constructor, always enforces). These two cases still confirm no
// division-by-zero crash results from equal thresholds.
TEST(MetricColorPolicyTest, ZeroRangeNormalDoesNotDivideByZero) {
    EXPECT_EQ(MetricColorPolicy::normalizedPercent(70, 50.0f, 50.0f, false), 100);
}

TEST(MetricColorPolicyTest, ZeroRangeReverseDoesNotDivideByZero) {
    EXPECT_EQ(MetricColorPolicy::normalizedPercent(70, 50.0f, 50.0f, true), 0);
}

TEST(MetricColorPolicyTest, ValueExactlyAtLowerThresholdIsZero) {
    EXPECT_EQ(MetricColorPolicy::normalizedPercent(50, 50.0f, 90.0f, false), 0);
}

TEST(MetricColorPolicyTest, ValueExactlyAtUpperThresholdIsMax) {
    EXPECT_EQ(MetricColorPolicy::normalizedPercent(90, 50.0f, 90.0f, false), 100);
}
