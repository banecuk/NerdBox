#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>

// Incrementally parses Server-Sent Events out of an arbitrary byte stream.
// feed() may be called with any chunk size the underlying transport hands
// over — including a single byte, or a chunk that splits a line across two
// calls — accumulating state between calls. Pure logic, no Arduino/network
// dependency, so it is host-testable under the `native` PlatformIO env.
class SseEventParser {
 public:
    struct Event {
        static constexpr size_t kMaxEventNameLen = 31;
        char eventName[kMaxEventNameLen + 1] = "";  // "" if the source omitted `event:`
        const char* data = "";  // points into this parser's internal buffer;
                                // only valid until the next feed() call
        size_t dataLen = 0;
    };

    using EventCallback = std::function<void(const Event&)>;

    // capacityBytes bounds both a single accumulated line and the joined
    // `data:` payload. Either overflowing drops the offending line/event
    // (see overflowCount()) rather than corrupting or silently truncating.
    explicit SseEventParser(size_t capacityBytes);

    void feed(const char* data, size_t len, const EventCallback& onEvent);
    void reset();
    uint32_t overflowCount() const { return overflowCount_; }

 private:
    void endOfLine(const EventCallback& onEvent);
    void dispatchIfReady(const EventCallback& onEvent);
    void parseLine(const char* line, size_t len);
    void appendData(const char* chunk, size_t len);

    std::unique_ptr<char[]> lineBuf_;
    std::unique_ptr<char[]> dataBuf_;
    size_t capacity_;

    size_t lineLen_ = 0;
    bool lineOverflowed_ = false;

    size_t dataLen_ = 0;
    bool dataOverflowed_ = false;

    char pendingEventName_[Event::kMaxEventNameLen + 1] = "";
    uint32_t overflowCount_ = 0;
};
