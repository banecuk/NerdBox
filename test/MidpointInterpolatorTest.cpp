#include "MidpointInterpolator.h"

#include <gtest/gtest.h>

TEST(MidpointInterpolatorTest, FirstSampleShowsActualValueImmediately) {
    MidpointInterpolator interp;
    interp.onSample(10.0f, 1000);
    EXPECT_FLOAT_EQ(interp.displayValue(1000), 10.0f);
    EXPECT_FALSE(interp.isPending(1000));
}

TEST(MidpointInterpolatorTest, ShowsAverageRightAfterASecondSampleArrives) {
    MidpointInterpolator interp;
    interp.onSample(10.0f, 1000);
    interp.onSample(20.0f, 2000);  // 1000ms since previous arrival
    EXPECT_TRUE(interp.isPending(2000));
    EXPECT_FLOAT_EQ(interp.displayValue(2000), 15.0f);
}

TEST(MidpointInterpolatorTest, StillAverageJustBeforeTheHalfwayDeadline) {
    MidpointInterpolator interp;
    interp.onSample(10.0f, 1000);
    interp.onSample(20.0f, 2000);
    EXPECT_TRUE(interp.isPending(2499));
    EXPECT_FLOAT_EQ(interp.displayValue(2499), 15.0f);
}

TEST(MidpointInterpolatorTest, SnapsToActualValueAtTheHalfwayDeadline) {
    MidpointInterpolator interp;
    interp.onSample(10.0f, 1000);
    interp.onSample(20.0f, 2000);
    EXPECT_FALSE(interp.isPending(2500));
    EXPECT_FLOAT_EQ(interp.displayValue(2500), 20.0f);
}

TEST(MidpointInterpolatorTest, StaysAtActualValueAfterTheDeadline) {
    MidpointInterpolator interp;
    interp.onSample(10.0f, 1000);
    interp.onSample(20.0f, 2000);
    EXPECT_FLOAT_EQ(interp.displayValue(9000), 20.0f);
}

TEST(MidpointInterpolatorTest, ConsumeRevealDueFiresExactlyOncePerCycle) {
    MidpointInterpolator interp;
    interp.onSample(10.0f, 1000);
    interp.onSample(20.0f, 2000);  // reveal deadline at 2500

    EXPECT_FALSE(interp.consumeRevealDue(2000));
    EXPECT_FALSE(interp.consumeRevealDue(2499));
    EXPECT_TRUE(interp.consumeRevealDue(2500));
    // One-shot: repeat calls after the deadline don't fire again.
    EXPECT_FALSE(interp.consumeRevealDue(2600));
    EXPECT_FALSE(interp.consumeRevealDue(9000));
}

TEST(MidpointInterpolatorTest, ConsumeRevealDueResetsOnEverySample) {
    MidpointInterpolator interp;
    interp.onSample(10.0f, 1000);
    interp.onSample(20.0f, 2000);  // reveal deadline at 2500
    EXPECT_TRUE(interp.consumeRevealDue(2500));

    interp.onSample(30.0f, 3000);  // reveal deadline at 3500
    EXPECT_FALSE(interp.consumeRevealDue(3000));
    EXPECT_TRUE(interp.consumeRevealDue(3500));
}

// First sample has no prior arrival to derive an interval from, so it must
// not be treated as "pending" — there's nothing to average against.
TEST(MidpointInterpolatorTest, FirstSampleNeverConsumesARevealDeadline) {
    MidpointInterpolator interp;
    interp.onSample(10.0f, 1000);
    EXPECT_FALSE(interp.consumeRevealDue(1000));
    EXPECT_FALSE(interp.consumeRevealDue(5000));
}
