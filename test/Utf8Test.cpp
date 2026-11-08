#include <cstring>

#include "utils/Utf8.h"

#include <gtest/gtest.h>

TEST(Utf8Test, ShortStringCopiesWhole) {
    char buf[16];
    EXPECT_EQ(Utf8::copyTruncated(buf, sizeof(buf), "hello"), 5u);
    EXPECT_STREQ(buf, "hello");
}

TEST(Utf8Test, ExactFitCopiesWhole) {
    char buf[6];
    EXPECT_EQ(Utf8::copyTruncated(buf, sizeof(buf), "hello"), 5u);
    EXPECT_STREQ(buf, "hello");
}

TEST(Utf8Test, AsciiTruncatesAtCapacity) {
    char buf[4];
    EXPECT_EQ(Utf8::copyTruncated(buf, sizeof(buf), "hello"), 3u);
    EXPECT_STREQ(buf, "hel");
}

TEST(Utf8Test, TwoByteSequenceIsNotSplit) {
    char buf[4];  // room for 3 bytes: "a" + "Ж" (2) fits; "aЖЖ" (5) doesn't
    EXPECT_EQ(Utf8::copyTruncated(buf, sizeof(buf), "a\xD0\x96\xD0\x96"), 3u);
    EXPECT_STREQ(buf, "a\xD0\x96");

    char buf2[3];  // room for 2 bytes: "a" + half of "Ж" → just "a"
    EXPECT_EQ(Utf8::copyTruncated(buf2, sizeof(buf2), "a\xD0\x96"), 1u);
    EXPECT_STREQ(buf2, "a");
}

TEST(Utf8Test, ThreeByteSequenceIsNotSplit) {
    // "日本" = E6 97 A5 E6 9C AC
    const char* src = "\xE6\x97\xA5\xE6\x9C\xAC";
    char buf[6];  // 5 bytes: first char + 2 of the second → cut to 3
    EXPECT_EQ(Utf8::copyTruncated(buf, sizeof(buf), src), 3u);
    EXPECT_STREQ(buf, "\xE6\x97\xA5");
}

TEST(Utf8Test, FourByteSequenceIsNotSplit) {
    // U+1F3B5 (musical note) = F0 9F 8E B5
    const char* src = "x\xF0\x9F\x8E\xB5";
    for (size_t cap = 2; cap <= 5; ++cap) {
        char buf[8];
        EXPECT_EQ(Utf8::copyTruncated(buf, cap, src), 1u) << "cap " << cap;
        EXPECT_STREQ(buf, "x");
    }
    char buf[6];
    EXPECT_EQ(Utf8::copyTruncated(buf, sizeof(buf), src), 5u);
}

TEST(Utf8Test, NullAndZeroCapacityAreSafe) {
    char buf[4] = {'z', 'z', 'z', 'z'};
    EXPECT_EQ(Utf8::copyTruncated(buf, sizeof(buf), nullptr), 0u);
    EXPECT_STREQ(buf, "");
    EXPECT_EQ(Utf8::copyTruncated(buf, 0, "abc"), 0u);
    EXPECT_EQ(Utf8::copyTruncated(nullptr, 4, "abc"), 0u);
    char one[1];
    EXPECT_EQ(Utf8::copyTruncated(one, 1, "abc"), 0u);
    EXPECT_STREQ(one, "");
}
