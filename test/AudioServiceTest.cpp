#include <cstring>
#include <string>

#include "NullLogger.h"
#include "services/audio/AudioData.h"
#include "services/audio/AudioService.h"

#include <gtest/gtest.h>

namespace {

class AudioServiceTest : public ::testing::Test {
 protected:
    AudioData data;
    NullLogger logger;
    AudioService service{data, logger};

    bool push(const std::string& json, uint32_t nowMs = 1000) {
        return service.handlePush(json.c_str(), json.size(), nowMs);
    }

    static std::string track(uint32_t seq, uint32_t trackId, uint32_t session = 7,
                             const std::string& title = "Song") {
        return R"({"event":"track","seq":)" + std::to_string(seq) + R"(,"trackId":)" +
               std::to_string(trackId) + R"(,"session":)" + std::to_string(session) +
               R"(,"title":")" + title +
               R"(","artist":"Artist","album":"Album","format":"flac","bitrate":"1000",)"
               R"("durationMs":200000,"positionMs":0,"playState":"Playing","isPlaying":true})";
    }

    static std::string simple(const char* event, uint32_t seq, uint32_t trackId,
                              uint32_t session = 7, uint32_t positionMs = 5000) {
        return std::string(R"({"event":")") + event + R"(","seq":)" + std::to_string(seq) +
               R"(,"trackId":)" + std::to_string(trackId) + R"(,"session":)" +
               std::to_string(session) + R"(,"positionMs":)" + std::to_string(positionMs) + "}";
    }
};

}  // namespace

TEST_F(AudioServiceTest, FirstTrackEventAppliesMetadataWithoutResync) {
    EXPECT_FALSE(push(track(1, 42)));
    EXPECT_TRUE(data.hasTrack);
    EXPECT_STREQ(data.title, "Song");
    EXPECT_STREQ(data.artist, "Artist");
    EXPECT_STREQ(data.format, "flac");
    EXPECT_EQ(data.durationMs, 200000u);
    EXPECT_EQ(data.playState, AudioData::PlayState::Playing);
    EXPECT_TRUE(data.isPlaying);
    EXPECT_EQ(data.seq, 1u);
    EXPECT_TRUE(data.freshness.available());
    EXPECT_EQ(data.freshness.lastUpdateMs(), 1000u);
}

TEST_F(AudioServiceTest, ContiguousPingDoesNotResync) {
    push(track(1, 42));
    EXPECT_FALSE(push(simple("ping", 2, 42, 7, 12000)));
    EXPECT_EQ(data.positionMs, 12000u);
    EXPECT_TRUE(data.isPlaying);
}

TEST_F(AudioServiceTest, SeqGapRequestsResync) {
    push(track(1, 42));
    EXPECT_TRUE(push(simple("ping", 3, 42)));
    // The event is still applied; the resync only asks for a fresh track.
    EXPECT_EQ(data.seq, 3u);
}

TEST_F(AudioServiceTest, SessionChangeIsNotAGap) {
    push(track(10, 42, 7));
    EXPECT_FALSE(push(track(1, 1, 8)));
    EXPECT_EQ(data.session, 8u);
}

TEST_F(AudioServiceTest, PingWithMismatchedTrackIdRequestsResync) {
    push(track(1, 42));
    EXPECT_TRUE(push(simple("ping", 2, 43)));
}

TEST_F(AudioServiceTest, PausedWithMismatchedTrackIdRequestsResync) {
    push(track(1, 42));
    EXPECT_TRUE(push(simple("paused", 2, 43)));
    EXPECT_EQ(data.playState, AudioData::PlayState::Paused);
}

TEST_F(AudioServiceTest, MismatchKeepsRequestingUntilATrackEventArrives) {
    push(track(1, 42));
    EXPECT_TRUE(push(simple("ping", 2, 43)));
    // data_.trackId now reads 43, but the cached track is still 42's — a
    // second contiguous ping must not be accepted as matching.
    EXPECT_TRUE(push(simple("ping", 3, 43)));
    EXPECT_FALSE(push(track(4, 43)));
    EXPECT_FALSE(push(simple("ping", 5, 43)));
}

TEST_F(AudioServiceTest, HeartbeatWithNoCachedTrackRequestsResync) {
    // Device rebooted mid-playback: the first event it ever sees is a ping.
    EXPECT_TRUE(push(simple("ping", 5, 42)));
    EXPECT_FALSE(data.hasTrack);
}

TEST_F(AudioServiceTest, MalformedBodyRequestsResyncAndChangesNothing) {
    push(track(1, 42));
    EXPECT_TRUE(push("{not json", 2000));
    EXPECT_EQ(data.seq, 1u);
    EXPECT_EQ(data.freshness.lastUpdateMs(), 1000u);
}

TEST_F(AudioServiceTest, EmptyBodyRequestsResync) {
    EXPECT_TRUE(service.handlePush(nullptr, 0, 1000));
    EXPECT_TRUE(push(""));
}

TEST_F(AudioServiceTest, UnknownEventIsIgnoredWithoutTouchingBookkeeping) {
    push(track(1, 42), 1000);
    EXPECT_FALSE(push(simple("rewind", 2, 99, 9), 5000));
    EXPECT_EQ(data.seq, 1u);
    EXPECT_EQ(data.trackId, 42u);
    EXPECT_EQ(data.session, 7u);
    EXPECT_EQ(data.freshness.lastUpdateMs(), 1000u);
    EXPECT_NE(logger.lastWarning().find("rewind"), std::string::npos);
    // seq 2 is still the expected next one.
    EXPECT_FALSE(push(simple("ping", 2, 42)));
}

TEST_F(AudioServiceTest, StopAndOfflineStampInjectedTime) {
    push(track(1, 42));
    push(simple("stop", 2, 42), 7777);
    EXPECT_TRUE(data.stopped);
    EXPECT_FALSE(data.offline);
    EXPECT_EQ(data.stoppedAtMs, 7777u);

    push(simple("offline", 3, 42), 8888);
    EXPECT_TRUE(data.stopped);
    EXPECT_TRUE(data.offline);
    EXPECT_EQ(data.stoppedAtMs, 8888u);

    push(track(4, 43));
    EXPECT_FALSE(data.stopped);
    EXPECT_FALSE(data.offline);
}

TEST_F(AudioServiceTest, BodyNeedNotBeNulTerminated) {
    const std::string json = track(1, 42) + "GARBAGE";
    EXPECT_FALSE(service.handlePush(json.c_str(), json.size() - 7, 1000));
    EXPECT_STREQ(data.title, "Song");
}

TEST_F(AudioServiceTest, LongUtf8TitleIsCutOnCodePointBoundary) {
    // 60 x "Ж" (2 bytes each) = 120 bytes; title[] holds 95 + NUL.
    std::string title;
    for (int i = 0; i < 60; ++i) {
        title += "\xD0\x96";
    }
    push(track(1, 42, 7, title));
    const size_t len = strlen(data.title);
    EXPECT_EQ(len, 94u);  // 47 whole code points, not 95 bytes with a dangling lead
    EXPECT_EQ(len % 2, 0u);
}
