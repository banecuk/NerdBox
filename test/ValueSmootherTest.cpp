#include <gtest/gtest.h>

#include "ValueSmoother.h"

// ─── Helpers ────────────────────────────────────────────────────────────────

// Run N identical updates and return the final smoothed value at index 0.
static uint8_t converge(ValueSmoother& s, uint8_t value, int iterations = 40) {
    std::vector<uint8_t> v = {value};
    for (int i = 0; i < iterations; ++i) {
        s.update(v);
    }
    return s.getSmoothedValue(0);
}

// ─── Construction ───────────────────────────────────────────────────────────

TEST(ValueSmootherTest, ConstructsWithValidParams) {
    EXPECT_NO_THROW(ValueSmoother(4, 0.3f, 0.1f));
}

TEST(ValueSmootherTest, SizeMatchesConstructorArg) {
    ValueSmoother s(6, 0.5f, 0.5f);
    EXPECT_EQ(s.size(), 6u);
}

TEST(ValueSmootherTest, HasNoPreviousOnConstruction) {
    ValueSmoother s(2, 0.5f, 0.5f);
    EXPECT_FALSE(s.hasPrevious());
}

// ─── First update — seed behaviour ──────────────────────────────────────────

TEST(ValueSmootherTest, FirstUpdateSeedsWithRawValues) {
    ValueSmoother s(3, 0.5f, 0.5f);
    std::vector<uint8_t> input = {10, 50, 200};
    s.update(input);

    // On the first call the smoother must return the exact raw values
    // (no smoothing applied) and hasPrevious must flip to true.
    EXPECT_EQ(s.getSmoothedValue(0), 10);
    EXPECT_EQ(s.getSmoothedValue(1), 50);
    EXPECT_EQ(s.getSmoothedValue(2), 200);
    EXPECT_TRUE(s.hasPrevious());
}

// ─── Steady-state convergence ────────────────────────────────────────────────

TEST(ValueSmootherTest, ConvergesOnConstantInput) {
    ValueSmoother s(1, 0.3f, 0.3f);
    EXPECT_EQ(converge(s, 80), 80);
}

TEST(ValueSmootherTest, ConvergesFromZeroToTarget) {
    ValueSmoother s(1, 0.5f, 0.5f);
    // Seed at 0 by seeding first, then drive toward 100.
    std::vector<uint8_t> seed = {0};
    s.update(seed);
    EXPECT_EQ(converge(s, 100), 100);
}

// ─── Directional smoothing ───────────────────────────────────────────────────

// With upwardSmoothing = 1.0 the new value is accepted instantly.
// With downwardSmoothing = 0.0 the old value is never released.

TEST(ValueSmootherTest, InstantUpwardWithAlphaOne) {
    ValueSmoother s(1, /*up=*/1.0f, /*down=*/0.5f);
    std::vector<uint8_t> seed = {10};
    s.update(seed);                         // seed at 10

    std::vector<uint8_t> jump = {90};
    s.update(jump);                         // one step upward, alpha = 1.0

    EXPECT_EQ(s.getSmoothedValue(0), 90);   // must arrive immediately
}

TEST(ValueSmootherTest, NoDecayWithDownwardAlphaZero) {
    ValueSmoother s(1, /*up=*/0.5f, /*down=*/0.0f);
    std::vector<uint8_t> seed = {80};
    s.update(seed);                         // seed at 80

    std::vector<uint8_t> drop = {10};
    s.update(drop);                         // downward move, alpha = 0 → no change
    s.update(drop);

    EXPECT_EQ(s.getSmoothedValue(0), 80);   // value must not have moved
}

TEST(ValueSmootherTest, SlowerRiseWithLowUpwardAlpha) {
    // alpha_up = 0.1 (slow), alpha_down = 0.9 (fast)
    // After one step upward the smoothed value should be well below the target.
    ValueSmoother s(1, 0.1f, 0.9f);
    std::vector<uint8_t> seed = {0};
    s.update(seed);

    std::vector<uint8_t> target = {100};
    s.update(target);   // smoothed ≈ 0*0.9 + 100*0.1 = 10

    EXPECT_LT(s.getSmoothedValue(0), 20);
}

TEST(ValueSmootherTest, SlowerFallWithLowDownwardAlpha) {
    ValueSmoother s(1, 0.9f, 0.1f);
    std::vector<uint8_t> seed = {100};
    s.update(seed);

    std::vector<uint8_t> drop = {0};
    s.update(drop);   // smoothed ≈ 100*0.9 + 0*0.1 = 90

    EXPECT_GT(s.getSmoothedValue(0), 80);
}

// ─── Multi-channel independence ───────────────────────────────────────────────

TEST(ValueSmootherTest, EachChannelSmoothedIndependently) {
    ValueSmoother s(3, 1.0f, 1.0f);        // alpha = 1 → instant, for predictability
    std::vector<uint8_t> a = {10, 20, 30};
    std::vector<uint8_t> b = {90, 80, 70};
    s.update(a);
    s.update(b);

    EXPECT_EQ(s.getSmoothedValue(0), 90);
    EXPECT_EQ(s.getSmoothedValue(1), 80);
    EXPECT_EQ(s.getSmoothedValue(2), 70);
}

// ─── Input shorter than smoother size ────────────────────────────────────────

TEST(ValueSmootherTest, PartialUpdateOnlyAffectsProvidedChannels) {
    ValueSmoother s(4, 1.0f, 1.0f);
    std::vector<uint8_t> full  = {50, 50, 50, 50};
    std::vector<uint8_t> partial = {99, 99};    // only first two channels

    s.update(full);
    s.update(partial);

    EXPECT_EQ(s.getSmoothedValue(0), 99);
    EXPECT_EQ(s.getSmoothedValue(1), 99);
    // Channels 2 and 3 were not touched — must still hold previous value.
    EXPECT_EQ(s.getSmoothedValue(2), 50);
    EXPECT_EQ(s.getSmoothedValue(3), 50);
}

// ─── Pointer-based overload ───────────────────────────────────────────────────

TEST(ValueSmootherTest, RawPointerUpdateMatchesVectorOverload) {
    ValueSmoother sa(2, 0.4f, 0.4f);
    ValueSmoother sb(2, 0.4f, 0.4f);

    uint8_t arr[] = {30, 70};
    std::vector<uint8_t> vec = {30, 70};

    sa.update(arr, 2);
    sb.update(vec);

    // Both paths must produce the same result.
    EXPECT_EQ(sa.getSmoothedValue(0), sb.getSmoothedValue(0));
    EXPECT_EQ(sa.getSmoothedValue(1), sb.getSmoothedValue(1));
}

// ─── getSmoothedValues (bulk read) ───────────────────────────────────────────

TEST(ValueSmootherTest, BulkReadMatchesIndividualGet) {
    ValueSmoother s(3, 0.5f, 0.5f);
    std::vector<uint8_t> input = {10, 20, 30};
    s.update(input);

    std::vector<uint8_t> out;
    s.getSmoothedValues(out);

    ASSERT_EQ(out.size(), 3u);
    for (size_t i = 0; i < 3; ++i) {
        EXPECT_EQ(out[i], s.getSmoothedValue(i));
    }
}

TEST(ValueSmootherTest, BulkReadPointerMatchesVectorRead) {
    ValueSmoother s(3, 0.5f, 0.5f);
    std::vector<uint8_t> input = {15, 25, 35};
    s.update(input);

    uint8_t arr[3] = {};
    std::vector<uint8_t> vec;
    s.getSmoothedValues(arr, 3);
    s.getSmoothedValues(vec);

    for (size_t i = 0; i < 3; ++i) {
        EXPECT_EQ(arr[i], vec[i]);
    }
}

// ─── Reset ───────────────────────────────────────────────────────────────────

TEST(ValueSmootherTest, ResetClearsHasPreviousFlag) {
    ValueSmoother s(2, 0.5f, 0.5f);
    std::vector<uint8_t> input = {50, 50};
    s.update(input);
    EXPECT_TRUE(s.hasPrevious());

    s.reset();
    EXPECT_FALSE(s.hasPrevious());
}

TEST(ValueSmootherTest, ResetToZeroAllChannels) {
    ValueSmoother s(2, 1.0f, 1.0f);
    std::vector<uint8_t> input = {100, 200};
    s.update(input);
    s.reset();

    // After reset, seed with 0 — must come back as 0, confirming internal
    // state was wiped (not just the hasPrevious_ flag).
    std::vector<uint8_t> zeros = {0, 0};
    s.update(zeros);

    EXPECT_EQ(s.getSmoothedValue(0), 0);
    EXPECT_EQ(s.getSmoothedValue(1), 0);
}

TEST(ValueSmootherTest, ResetAllowsReseedFromFreshState) {
    ValueSmoother s(1, 0.5f, 0.5f);
    std::vector<uint8_t> high = {200};
    s.update(high);
    s.reset();

    // After reset the first update must seed cleanly, not blend with the
    // pre-reset value.
    std::vector<uint8_t> low = {10};
    s.update(low);

    EXPECT_EQ(s.getSmoothedValue(0), 10);
}

// ─── Boundary values ─────────────────────────────────────────────────────────

TEST(ValueSmootherTest, HandlesMinMaxUint8Values) {
    ValueSmoother s(2, 1.0f, 1.0f);
    std::vector<uint8_t> extremes = {0, 255};
    s.update(extremes);

    EXPECT_EQ(s.getSmoothedValue(0), 0);
    EXPECT_EQ(s.getSmoothedValue(1), 255);
}

TEST(ValueSmootherTest, SmoothedValueClampsToUint8Range) {
    // With alpha = 1.0 and value 255, the result must not overflow uint8_t.
    ValueSmoother s(1, 1.0f, 1.0f);
    std::vector<uint8_t> max = {255};
    s.update(max);
    s.update(max);

    // getSmoothedValue casts float → uint8_t; ensure no wraparound.
    EXPECT_EQ(s.getSmoothedValue(0), 255);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
