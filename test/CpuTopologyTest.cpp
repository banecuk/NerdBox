#include "CpuTopology.h"

#include <gtest/gtest.h>

using CpuTopology::classOf;
using CpuTopology::CoreClass;
using CpuTopology::hasSplit;
using CpuTopology::splitIndex;

TEST(CpuTopologyTest, FourteenSeventyKBoundary) {
    EXPECT_TRUE(hasSplit(16, 28));
    EXPECT_EQ(splitIndex(16, 28), 16);
    for (uint8_t i = 0; i < 16; ++i) {
        EXPECT_EQ(classOf(i, 16, 28), CoreClass::Performance) << "index " << int(i);
    }
    for (uint8_t i = 16; i < 28; ++i) {
        EXPECT_EQ(classOf(i, 16, 28), CoreClass::Efficiency) << "index " << int(i);
    }
}

TEST(CpuTopologyTest, ZeroPerformanceCountDisablesSplit) {
    EXPECT_FALSE(hasSplit(0, 28));
    EXPECT_EQ(splitIndex(0, 28), 0);
    EXPECT_EQ(classOf(0, 0, 28), CoreClass::Performance);
    EXPECT_EQ(classOf(27, 0, 28), CoreClass::Performance);
}

TEST(CpuTopologyTest, PerformanceCountEqualToTotalDisablesSplit) {
    EXPECT_FALSE(hasSplit(16, 16));
    EXPECT_EQ(splitIndex(16, 16), 0);
    EXPECT_EQ(classOf(15, 16, 16), CoreClass::Performance);
}

TEST(CpuTopologyTest, PerformanceCountAboveTotalDisablesSplit) {
    EXPECT_FALSE(hasSplit(16, 12));
    EXPECT_EQ(splitIndex(16, 12), 0);
    for (uint8_t i = 0; i < 12; ++i) {
        EXPECT_EQ(classOf(i, 16, 12), CoreClass::Performance);
    }
}

TEST(CpuTopologyTest, DegenerateTotalThreadCounts) {
    EXPECT_FALSE(hasSplit(16, 1));
    EXPECT_EQ(classOf(0, 16, 1), CoreClass::Performance);

    EXPECT_FALSE(hasSplit(16, 0));
    EXPECT_EQ(classOf(0, 16, 0), CoreClass::Performance);
}

TEST(CpuTopologyTest, BoundaryExactness) {
    EXPECT_EQ(classOf(15, 16, 28), CoreClass::Performance);
    EXPECT_EQ(classOf(16, 16, 28), CoreClass::Efficiency);
}
