#include "SseEventParser.h"

#include <cstring>

SseEventParser::SseEventParser(size_t capacityBytes)
    : lineBuf_(std::make_unique<char[]>(capacityBytes)),
      dataBuf_(std::make_unique<char[]>(capacityBytes)),
      capacity_(capacityBytes) {}

void SseEventParser::reset() {
    lineLen_ = 0;
    lineOverflowed_ = false;
    dataLen_ = 0;
    dataOverflowed_ = false;
    pendingEventName_[0] = '\0';
}

void SseEventParser::feed(const char* data, size_t len, const EventCallback& onEvent) {
    for (size_t i = 0; i < len; i++) {
        const char c = data[i];
        if (c == '\r') {
            continue;  // normalize CRLF -> LF; a bare CR is dropped too
        }
        if (c == '\n') {
            endOfLine(onEvent);
            continue;
        }
        if (lineLen_ < capacity_) {
            lineBuf_[lineLen_++] = c;
        } else {
            lineOverflowed_ = true;
        }
    }
}

void SseEventParser::endOfLine(const EventCallback& onEvent) {
    const size_t lineLen = lineLen_;
    const bool overflowed = lineOverflowed_;
    lineLen_ = 0;
    lineOverflowed_ = false;

    if (overflowed) {
        overflowCount_++;
        return;  // drop the oversized line; keep parsing subsequent lines
    }

    if (lineLen == 0) {
        dispatchIfReady(onEvent);
        return;
    }

    parseLine(lineBuf_.get(), lineLen);
}

void SseEventParser::parseLine(const char* line, size_t len) {
    // SSE field syntax: "<field>:<value>" with an optional single space
    // after the colon. Lines starting with ':' are comments (e.g.
    // keep-alive pings) and are ignored.
    if (line[0] == ':') {
        return;
    }

    auto startsWith = [&](const char* prefix, size_t prefixLen) {
        return len >= prefixLen && strncmp(line, prefix, prefixLen) == 0;
    };

    if (startsWith("data:", 5)) {
        const char* value = line + 5;
        size_t valueLen = len - 5;
        if (valueLen > 0 && value[0] == ' ') {
            value++;
            valueLen--;
        }
        appendData(value, valueLen);
        return;
    }

    if (startsWith("event:", 6)) {
        const char* value = line + 6;
        size_t valueLen = len - 6;
        if (valueLen > 0 && value[0] == ' ') {
            value++;
            valueLen--;
        }
        const size_t copyLen =
            valueLen < Event::kMaxEventNameLen ? valueLen : Event::kMaxEventNameLen;
        memcpy(pendingEventName_, value, copyLen);
        pendingEventName_[copyLen] = '\0';
        return;
    }

    // Other fields (id:, retry:) are recognized-but-unused by this client.
}

void SseEventParser::appendData(const char* chunk, size_t len) {
    if (dataOverflowed_) {
        return;  // already dropped this event's data; wait for the blank line to reset
    }

    // Multiple `data:` lines in one event are joined with '\n', per the SSE
    // spec — this is what makes multi-line JSON payloads reassemble correctly.
    const size_t separatorLen = (dataLen_ > 0) ? 1 : 0;
    if (dataLen_ + separatorLen + len >= capacity_) {
        dataOverflowed_ = true;
        overflowCount_++;
        return;
    }

    if (separatorLen) {
        dataBuf_[dataLen_++] = '\n';
    }
    memcpy(dataBuf_.get() + dataLen_, chunk, len);
    dataLen_ += len;
    dataBuf_[dataLen_] = '\0';  // convenience null terminator; dataLen_ is the real length
}

void SseEventParser::dispatchIfReady(const EventCallback& onEvent) {
    if (dataOverflowed_) {
        // Drop the whole event — a partial/corrupt payload is worse than a
        // missed one; the next tick's event recovers.
        dataOverflowed_ = false;
        dataLen_ = 0;
        pendingEventName_[0] = '\0';
        return;
    }

    if (dataLen_ == 0) {
        pendingEventName_[0] = '\0';
        return;  // blank line with nothing accumulated (e.g. keep-alive) — no-op
    }

    Event evt;
    strncpy(evt.eventName, pendingEventName_, Event::kMaxEventNameLen);
    evt.eventName[Event::kMaxEventNameLen] = '\0';
    evt.data = dataBuf_.get();
    evt.dataLen = dataLen_;

    onEvent(evt);

    dataLen_ = 0;
    pendingEventName_[0] = '\0';
}
