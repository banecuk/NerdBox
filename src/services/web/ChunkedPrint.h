#pragma once

#include <Print.h>
#include <WebServer.h>

#include <algorithm>
#include <cstring>

// Adapts WebServer's chunked-transfer sendContent() to Arduino's Print
// interface, so ArduinoJson's serializeJson(doc, Print&) can stream straight
// to the client through a small fixed buffer instead of building one large
// String/JsonDocument-to-String conversion.
class ChunkedPrint : public Print {
 public:
    explicit ChunkedPrint(WebServer& server) : server_(server) {}
    ~ChunkedPrint() override { flush(); }

    size_t write(uint8_t c) override { return write(&c, 1); }

    size_t write(const uint8_t* buffer, size_t size) override {
        size_t written = 0;
        while (written < size) {
            const size_t space = sizeof(buf_) - len_;
            const size_t chunk = std::min(space, size - written);
            memcpy(buf_ + len_, buffer + written, chunk);
            len_ += chunk;
            written += chunk;
            if (len_ == sizeof(buf_)) flushBuffer();
        }
        return size;
    }

    void flush() override { flushBuffer(); }

 private:
    void flushBuffer() {
        if (len_ > 0) {
            server_.sendContent(buf_, len_);
            len_ = 0;
        }
    }

    WebServer& server_;
    char buf_[256];
    size_t len_ = 0;
};
