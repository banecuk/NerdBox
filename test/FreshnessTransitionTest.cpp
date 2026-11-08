#include "utils/FreshnessTransition.h"

#include <gtest/gtest.h>

namespace {
unsigned long g_nowMs = 0;
unsigned long fakeMillis() {
    return g_nowMs;
}
using Edge = FreshnessTransition::Edge;
}  // namespace

class FreshnessTransitionTest : public ::testing::Test {
 protected:
    void SetUp() override { g_nowMs = 0; }
};

TEST_F(FreshnessTransitionTest, StartsNotFreshAndStalePollIsNoEdge) {
    PublishedFlag flag;
    FreshnessTransition t(flag, 5000, &fakeMillis);
    EXPECT_FALSE(t.fresh());
    EXPECT_EQ(t.poll(), Edge::None);
    EXPECT_FALSE(t.fresh());
}

TEST_F(FreshnessTransitionTest, FirstFreshPollReportsBecameFreshOnce) {
    PublishedFlag flag;
    g_nowMs = 100;
    flag.publish(g_nowMs);
    FreshnessTransition t(flag, 5000, &fakeMillis);
    EXPECT_EQ(t.poll(), Edge::BecameFresh);
    EXPECT_TRUE(t.fresh());
    EXPECT_EQ(t.poll(), Edge::None);
    EXPECT_TRUE(t.fresh());
}

TEST_F(FreshnessTransitionTest, AgingPastTimeoutReportsBecameStaleOnce) {
    PublishedFlag flag;
    flag.publish(0);
    FreshnessTransition t(flag, 5000, &fakeMillis);
    EXPECT_EQ(t.poll(), Edge::BecameFresh);
    g_nowMs = 5000;
    EXPECT_EQ(t.poll(), Edge::None);  // boundary is inclusive, like the guard
    g_nowMs = 5001;
    EXPECT_EQ(t.poll(), Edge::BecameStale);
    EXPECT_FALSE(t.fresh());
    EXPECT_EQ(t.poll(), Edge::None);
}

TEST_F(FreshnessTransitionTest, RepublishAfterStaleReportsBecameFresh) {
    PublishedFlag flag;
    flag.publish(0);
    FreshnessTransition t(flag, 5000, &fakeMillis);
    t.poll();
    g_nowMs = 9000;
    EXPECT_EQ(t.poll(), Edge::BecameStale);
    flag.publish(g_nowMs);
    EXPECT_EQ(t.poll(), Edge::BecameFresh);
}

TEST_F(FreshnessTransitionTest, HasContentFalseCountsAsStale) {
    PublishedFlag flag;
    flag.publish(0);
    FreshnessTransition t(flag, 5000, &fakeMillis);
    EXPECT_EQ(t.poll(false), Edge::None);
    EXPECT_FALSE(t.fresh());
    EXPECT_EQ(t.poll(true), Edge::BecameFresh);
    EXPECT_EQ(t.poll(false), Edge::BecameStale);
}

TEST_F(FreshnessTransitionTest, ResetMakesNextFreshPollAnEdgeAgain) {
    PublishedFlag flag;
    flag.publish(0);
    FreshnessTransition t(flag, 5000, &fakeMillis);
    EXPECT_EQ(t.poll(), Edge::BecameFresh);
    t.reset();
    EXPECT_FALSE(t.fresh());
    EXPECT_EQ(t.poll(), Edge::BecameFresh);
}

TEST_F(FreshnessTransitionTest, IsFreshNowDoesNotRecord) {
    PublishedFlag flag;
    flag.publish(0);
    FreshnessTransition t(flag, 5000, &fakeMillis);
    EXPECT_TRUE(t.isFreshNow());
    EXPECT_FALSE(t.fresh());
    EXPECT_EQ(t.poll(), Edge::BecameFresh);
}

TEST_F(FreshnessTransitionTest, SetTimeoutAppliesToLaterPolls) {
    PublishedFlag flag;
    flag.publish(0);
    FreshnessTransition t(flag, 5000, &fakeMillis);
    t.poll();
    g_nowMs = 3000;
    t.setTimeout(1000);
    EXPECT_EQ(t.poll(), Edge::BecameStale);
}
