#include "ui/widgets/base/MetricTileLayout.h"

#include <gtest/gtest.h>

using MetricTileLayout::HAlign;
using MetricTileLayout::Rect;

TEST(MetricTileLayoutTest, UnlabelledAreaHasMarginOnAllSides) {
    const Rect a = MetricTileLayout::valueArea({10, 20, 100, 40}, false, 30, 1, 2);
    EXPECT_EQ(a.x, 12);
    EXPECT_EQ(a.y, 22);
    EXPECT_EQ(a.w, 96);
    EXPECT_EQ(a.h, 36);
}

TEST(MetricTileLayoutTest, LabelledAreaStartsAfterLabelAndSeparator) {
    const Rect a = MetricTileLayout::valueArea({10, 20, 100, 40}, true, 30, 1, 2);
    EXPECT_EQ(a.x, 41);         // 10 + 30 label + 1 separator
    EXPECT_EQ(a.w, 67);         // 100 - 30 - 1 - 2 (right margin only)
    EXPECT_EQ(a.x + a.w, 108);  // right edge = tile right (110) - margin
    EXPECT_EQ(a.y, 22);
    EXPECT_EQ(a.h, 36);
}

TEST(MetricTileLayoutTest, ZeroMarginAreaFillsTile) {
    const Rect a = MetricTileLayout::valueArea({0, 0, 64, 24}, false, 0, 1, 0);
    EXPECT_EQ(a.x, 0);
    EXPECT_EQ(a.w, 64);
    EXPECT_EQ(a.h, 24);
}

TEST(MetricTileLayoutTest, StartXLeftAndRightInsetByMargin) {
    EXPECT_EQ(MetricTileLayout::startX(100, 80, 30, HAlign::Left, 10), 110);
    EXPECT_EQ(MetricTileLayout::startX(100, 80, 30, HAlign::Right, 10),
              140);  // ends at 170 = 180-10
}

TEST(MetricTileLayoutTest, StartXCenterUsesIntegerHalving) {
    EXPECT_EQ(MetricTileLayout::startX(100, 80, 30, HAlign::Center, 10), 125);
    // Odd widths: 81/2 - 31/2 = 40 - 15 — must stay exactly what the widget
    // drew before the extraction.
    EXPECT_EQ(MetricTileLayout::startX(100, 81, 31, HAlign::Center, 10), 125);
}

TEST(MetricTileLayoutTest, StartXOverwideBlockGoesLeftOfArea) {
    // Wider than the area: centred block starts before areaX; the clip rect,
    // not this function, keeps it in the tile.
    EXPECT_EQ(MetricTileLayout::startX(100, 40, 60, HAlign::Center, 10), 90);
}

TEST(MetricTileLayoutTest, NeedsClearWithoutUnitOnlyOnShrink) {
    EXPECT_FALSE(MetricTileLayout::needsClear(30, 30, 0));
    EXPECT_FALSE(MetricTileLayout::needsClear(40, 30, 0));
    EXPECT_TRUE(MetricTileLayout::needsClear(20, 30, 0));
}

TEST(MetricTileLayoutTest, NeedsClearWithUnitOnAnyChange) {
    EXPECT_FALSE(MetricTileLayout::needsClear(30, 30, 12));
    EXPECT_TRUE(MetricTileLayout::needsClear(40, 30, 12));
    EXPECT_TRUE(MetricTileLayout::needsClear(20, 30, 12));
}
