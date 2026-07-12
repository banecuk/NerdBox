#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "SseEventParser.h"

// ─── Helpers ────────────────────────────────────────────────────────────────

struct CapturedEvent {
    std::string eventName;
    std::string data;
};

static std::vector<CapturedEvent> feedAll(SseEventParser& p, const std::string& raw) {
    std::vector<CapturedEvent> events;
    p.feed(raw.data(), raw.size(), [&](const SseEventParser::Event& e) {
        events.push_back({std::string(e.eventName), std::string(e.data, e.dataLen)});
    });
    return events;
}

// Feeds the input one byte at a time — exercises the "line split across
// feed() calls" path that a real non-blocking socket read would produce.
static std::vector<CapturedEvent> feedByteByByte(SseEventParser& p, const std::string& raw) {
    std::vector<CapturedEvent> events;
    for (char c : raw) {
        p.feed(&c, 1, [&](const SseEventParser::Event& e) {
            events.push_back({std::string(e.eventName), std::string(e.data, e.dataLen)});
        });
    }
    return events;
}

// ─── Basic framing ──────────────────────────────────────────────────────────

TEST(SseEventParserTest, ParsesSingleDataOnlyEvent) {
    SseEventParser p(256);
    auto events = feedAll(p, "data: {\"a\":1}\n\n");

    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].data, "{\"a\":1}");
    EXPECT_EQ(events[0].eventName, "");
}

TEST(SseEventParserTest, ParsesEventNameAndData) {
    SseEventParser p(256);
    auto events = feedAll(p, "event: metrics\ndata: {\"a\":1}\n\n");

    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].eventName, "metrics");
    EXPECT_EQ(events[0].data, "{\"a\":1}");
}

TEST(SseEventParserTest, HandlesCrLfLineEndings) {
    SseEventParser p(256);
    auto events = feedAll(p, "event: metrics\r\ndata: {\"a\":1}\r\n\r\n");

    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].eventName, "metrics");
    EXPECT_EQ(events[0].data, "{\"a\":1}");
}

TEST(SseEventParserTest, MultipleEventsInOneFeedCall) {
    SseEventParser p(256);
    auto events = feedAll(p, "data: one\n\ndata: two\n\n");

    ASSERT_EQ(events.size(), 2u);
    EXPECT_EQ(events[0].data, "one");
    EXPECT_EQ(events[1].data, "two");
}

TEST(SseEventParserTest, BlankLineWithNoDataIsANoOp) {
    SseEventParser p(256);
    auto events = feedAll(p, "\n\n\ndata: real\n\n");

    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].data, "real");
}

TEST(SseEventParserTest, CommentLinesAreIgnored) {
    SseEventParser p(256);
    auto events = feedAll(p, ": keep-alive\ndata: real\n\n");

    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].data, "real");
}

// ─── Multi-line data reassembly ─────────────────────────────────────────────

TEST(SseEventParserTest, MultiLineDataJoinedWithNewline) {
    SseEventParser p(256);
    auto events = feedAll(p, "data: line1\ndata: line2\n\n");

    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].data, "line1\nline2");
}

// ─── Split across feed() calls ──────────────────────────────────────────────

TEST(SseEventParserTest, LineSplitAcrossTwoFeedCalls) {
    SseEventParser p(256);
    std::vector<CapturedEvent> events;
    auto onEvent = [&](const SseEventParser::Event& e) {
        events.push_back({std::string(e.eventName), std::string(e.data, e.dataLen)});
    };

    p.feed("data: hel", 9, onEvent);
    p.feed("lo\n\n", 4, onEvent);

    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].data, "hello");
}

TEST(SseEventParserTest, ByteByByteFeedProducesSameResultAsWholeChunk) {
    SseEventParser p(256);
    auto events = feedByteByByte(p, "event: metrics\ndata: {\"a\":1,\"b\":2}\n\n");

    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].eventName, "metrics");
    EXPECT_EQ(events[0].data, "{\"a\":1,\"b\":2}");
}

// ─── Buffer-full degradation ─────────────────────────────────────────────────

TEST(SseEventParserTest, OversizedLineIsDroppedNotCorrupting) {
    SseEventParser p(16);  // small capacity to force overflow
    std::string oversizedLine = "data: " + std::string(64, 'x');

    auto events = feedAll(p, oversizedLine + "\n\ndata: ok\n\n");

    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].data, "ok");
    EXPECT_GT(p.overflowCount(), 0u);
}

TEST(SseEventParserTest, OversizedDataDropsWholeEventButRecovers) {
    SseEventParser p(32);
    std::string bigPayload(64, 'y');

    auto events = feedAll(p, "data: " + bigPayload + "\n\ndata: small\n\n");

    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].data, "small");
    EXPECT_GT(p.overflowCount(), 0u);
}

// ─── Reset ───────────────────────────────────────────────────────────────────

TEST(SseEventParserTest, ResetClearsInProgressState) {
    SseEventParser p(256);
    std::vector<CapturedEvent> events;
    auto onEvent = [&](const SseEventParser::Event& e) {
        events.push_back({std::string(e.eventName), std::string(e.data, e.dataLen)});
    };

    p.feed("data: partial", 13, onEvent);  // no terminating blank line yet
    p.reset();
    p.feed("data: fresh\n\n", 13, onEvent);

    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].data, "fresh");
}

// main() is provided by ValueSmootherTest.cpp — PlatformIO's native env links
// all test/*.cpp into one binary, so only one TU may define it.
