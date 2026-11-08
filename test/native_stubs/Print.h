#pragma once

// Minimal stand-in for the Arduino core's Print class, present only on the
// `[env:native]` build path (see Arduino.h next to this file). Provides just
// the surface the host-tested web emitters (services/web/WebEmitters.cpp)
// use: write() and printf(). Never included on the real firmware build.

#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>

class Print {
 public:
    virtual ~Print() = default;

    virtual size_t write(uint8_t c) = 0;
    virtual size_t write(const uint8_t* buffer, size_t size) {
        size_t n = 0;
        while (size--) {
            n += write(*buffer++);
        }
        return n;
    }
    virtual void flush() {}

    size_t print(const char* s) {
        return write(reinterpret_cast<const uint8_t*>(s), std::char_traits<char>::length(s));
    }

    __attribute__((format(printf, 2, 3))) size_t printf(const char* format, ...) {
        char buf[256];
        va_list args;
        va_start(args, format);
        const int len = vsnprintf(buf, sizeof(buf), format, args);
        va_end(args);
        if (len < 0) {
            return 0;
        }
        if (static_cast<size_t>(len) < sizeof(buf)) {
            return write(reinterpret_cast<const uint8_t*>(buf), static_cast<size_t>(len));
        }
        std::string big(static_cast<size_t>(len) + 1, '\0');
        va_start(args, format);
        vsnprintf(&big[0], big.size(), format, args);
        va_end(args);
        return write(reinterpret_cast<const uint8_t*>(big.data()), static_cast<size_t>(len));
    }
};

// Host-test sink: accumulates everything written into a std::string.
class StringPrint : public Print {
 public:
    size_t write(uint8_t c) override {
        out_.push_back(static_cast<char>(c));
        return 1;
    }
    size_t write(const uint8_t* buffer, size_t size) override {
        out_.append(reinterpret_cast<const char*>(buffer), size);
        return size;
    }
    const std::string& str() const { return out_; }

 private:
    std::string out_;
};
