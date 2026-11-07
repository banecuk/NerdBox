#include "CachedString.h"

#include <gtest/gtest.h>

TEST(CachedStringTest, StartsEmpty) {
    CachedString<8> c;
    EXPECT_STREQ(c.c_str(), "");
    EXPECT_TRUE(c.equals(""));
}

TEST(CachedStringTest, SetIfChangedReportsFirstWrite) {
    CachedString<8> c;
    EXPECT_TRUE(c.setIfChanged("12"));
    EXPECT_STREQ(c.c_str(), "12");
}

TEST(CachedStringTest, SetIfChangedIgnoresSameValue) {
    CachedString<8> c;
    c.set("12");
    EXPECT_FALSE(c.setIfChanged("12"));
    EXPECT_STREQ(c.c_str(), "12");
}

TEST(CachedStringTest, SetIfChangedReportsDifferentValue) {
    CachedString<8> c;
    c.set("12");
    EXPECT_TRUE(c.setIfChanged("13"));
    EXPECT_STREQ(c.c_str(), "13");
}

TEST(CachedStringTest, TruncatesAndAlwaysTerminates) {
    CachedString<4> c;
    c.set("ABCDEFG");
    EXPECT_STREQ(c.c_str(), "ABC");
}

TEST(CachedStringTest, OverlongSourceComparesEqualToItsTruncation) {
    CachedString<4> c;
    EXPECT_TRUE(c.setIfChanged("ABCDEFG"));
    // Same overlong source again: must not read as a change every tick.
    EXPECT_FALSE(c.setIfChanged("ABCDEFG"));
}

TEST(CachedStringTest, ClearForcesNextChange) {
    CachedString<8> c;
    c.set("x");
    c.clear();
    EXPECT_TRUE(c.setIfChanged("x"));
}
