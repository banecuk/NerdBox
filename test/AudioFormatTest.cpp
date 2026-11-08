#include "ui/widgets/display/AudioFormat.h"

#include <gtest/gtest.h>

TEST(AudioFormatTest, FormatTimePadsSeconds) {
    char buf[16];
    AudioFormat::formatTime(0, buf, sizeof(buf));
    EXPECT_STREQ(buf, "0:00");
    AudioFormat::formatTime(65999, buf, sizeof(buf));
    EXPECT_STREQ(buf, "1:05");
    AudioFormat::formatTime(75u * 60u * 1000u, buf, sizeof(buf));
    EXPECT_STREQ(buf, "75:00");
}

TEST(AudioFormatTest, LosslessShowsNameOnly) {
    char buf[24];
    AudioFormat::formatQuality("flac", "1000", buf, sizeof(buf));
    EXPECT_STREQ(buf, "FLAC");
    AudioFormat::formatQuality("wav", "1411", buf, sizeof(buf));
    EXPECT_STREQ(buf, "WAV");
    AudioFormat::formatQuality("alac", "", buf, sizeof(buf));
    EXPECT_STREQ(buf, "ALAC");
    AudioFormat::formatQuality("ape", "900", buf, sizeof(buf));
    EXPECT_STREQ(buf, "APE");
}

TEST(AudioFormatTest, LossyPairsFormatWithBitrate) {
    char buf[24];
    AudioFormat::formatQuality("mp3", "256", buf, sizeof(buf));
    EXPECT_STREQ(buf, "MP3 256");
    AudioFormat::formatQuality("Ogg", "160", buf, sizeof(buf));
    EXPECT_STREQ(buf, "OGG 160");
}

TEST(AudioFormatTest, LossyWithoutBitrateShowsNameOnly) {
    char buf[24];
    AudioFormat::formatQuality("mp3", "", buf, sizeof(buf));
    EXPECT_STREQ(buf, "MP3");
    AudioFormat::formatQuality("mp3", nullptr, buf, sizeof(buf));
    EXPECT_STREQ(buf, "MP3");
}

TEST(AudioFormatTest, EmptyFormatIsEmpty) {
    char buf[24] = "junk";
    AudioFormat::formatQuality("", "320", buf, sizeof(buf));
    EXPECT_STREQ(buf, "");
    AudioFormat::formatQuality(nullptr, "320", buf, sizeof(buf));
    EXPECT_STREQ(buf, "");
}

TEST(AudioFormatTest, KnownLengthShowsElapsedAndTotal) {
    char buf[40];
    AudioFormat::formatTimeLine(83000, 200000, buf, sizeof(buf));
    EXPECT_STREQ(buf, "1:23 / 3:20");
}

TEST(AudioFormatTest, UnknownLengthShowsElapsedOnly) {
    // B30: a live stream used to read "1:23 / 0:00".
    char buf[40];
    AudioFormat::formatTimeLine(83000, 0, buf, sizeof(buf));
    EXPECT_STREQ(buf, "1:23");
}

TEST(AudioFormatTest, PositionPastEndIsClampedToDuration) {
    char buf[40];
    AudioFormat::formatTimeLine(250000, 200000, buf, sizeof(buf));
    EXPECT_STREQ(buf, "3:20 / 3:20");
    EXPECT_EQ(AudioFormat::clampPosition(250000, 0), 250000u);
}

TEST(AudioFormatTest, ProgressFillWidth) {
    EXPECT_EQ(AudioFormat::progressFillWidth(0, 200000, 300), 0);
    EXPECT_EQ(AudioFormat::progressFillWidth(100000, 200000, 300), 150);
    EXPECT_EQ(AudioFormat::progressFillWidth(200000, 200000, 300), 300);
    EXPECT_EQ(AudioFormat::progressFillWidth(999999, 200000, 300), 300);
    EXPECT_EQ(AudioFormat::progressFillWidth(100000, 0, 300), 0);
    EXPECT_EQ(AudioFormat::progressFillWidth(100000, 200000, 0), 0);
    // A multi-hour duration must not overflow the intermediate product.
    EXPECT_EQ(AudioFormat::progressFillWidth(3600000u * 5, 3600000u * 10, 400), 200);
}
