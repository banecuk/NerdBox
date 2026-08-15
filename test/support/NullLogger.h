#pragma once

#include <cstdarg>
#include <cstdio>
#include <string>

#include "LoggerInterface.h"

// No-op LoggerInterface implementation for host tests. The formatted (`f`-
// suffix) methods also stash their last rendered message (with the varargs
// substituted in) so a test can assert on it — e.g. PcMetricsParser's
// core-count-mismatch warning.
class NullLogger : public LoggerInterface {
 public:
    void debug(const char*, bool) override {}
    void info(const char*, bool) override {}
    void warning(const char*, bool) override {}
    void error(const char*, bool) override {}
    void critical(const char*, bool) override {}

    void debugf(const char* format, ...) override {
        va_list args;
        va_start(args, format);
        capture(lastDebug_, format, args);
        va_end(args);
    }
    void infof(const char* format, ...) override {
        va_list args;
        va_start(args, format);
        capture(lastInfo_, format, args);
        va_end(args);
    }
    void warningf(const char* format, ...) override {
        va_list args;
        va_start(args, format);
        capture(lastWarning_, format, args);
        va_end(args);
    }
    void errorf(const char* format, ...) override {
        va_list args;
        va_start(args, format);
        capture(lastError_, format, args);
        va_end(args);
    }
    void criticalf(const char* format, ...) override {
        va_list args;
        va_start(args, format);
        capture(lastCritical_, format, args);
        va_end(args);
    }

    const std::string& lastWarning() const { return lastWarning_; }
    const std::string& lastInfo() const { return lastInfo_; }
    const std::string& lastError() const { return lastError_; }

 private:
    static void capture(std::string& dest, const char* format, va_list args) {
        char buf[256];
        vsnprintf(buf, sizeof(buf), format, args);
        dest = buf;
    }

    std::string lastDebug_;
    std::string lastInfo_;
    std::string lastWarning_;
    std::string lastError_;
    std::string lastCritical_;
};
