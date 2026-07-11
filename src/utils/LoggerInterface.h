#pragma once

#include <Arduino.h>

#include <cstddef>

#include <stdarg.h>

class LoggerInterface {
 public:
    enum class LogLevel {
        DEBUG,    // For detailed debugging information
        INFO,     // General information
        WARNING,  // Potential issues
        ERROR,    // Error conditions
        CRITICAL  // Critical failures
    };

    struct LogEntry {
        char timestamp[12];   // "HH:MM:SS" + null — 9 chars, 12 is comfortable
        LogLevel level;
        char message[200];    // matches the screenBuffer size in Logger.cpp
        bool forScreen;
    };

    virtual ~LoggerInterface() = default;

    // Basic log methods. `const char*` overloads take priority over the
    // `String&` ones for string-literal call sites, so `logger_.info("x")`
    // never constructs a heap String.
    virtual void debug(const char* message, bool forScreen = false) = 0;
    virtual void info(const char* message, bool forScreen = false) = 0;
    virtual void warning(const char* message, bool forScreen = false) = 0;
    virtual void error(const char* message, bool forScreen = false) = 0;
    virtual void critical(const char* message, bool forScreen = false) = 0;

    virtual void debug(const String& message, bool forScreen = false) = 0;
    virtual void info(const String& message, bool forScreen = false) = 0;
    virtual void warning(const String& message, bool forScreen = false) = 0;
    virtual void error(const String& message, bool forScreen = false) = 0;
    virtual void critical(const String& message, bool forScreen = false) = 0;

    // Formatted log methods
    virtual void debugf(const char* format, ...) = 0;
    virtual void infof(const char* format, ...) = 0;
    virtual void warningf(const char* format, ...) = 0;
    virtual void errorf(const char* format, ...) = 0;
    virtual void criticalf(const char* format, ...) = 0;

    // Pop the oldest queued screen message into `buffer` (truncated to fit,
    // always null-terminated). Returns false if the queue is empty.
    virtual bool popScreenMessage(char* buffer, size_t bufferSize) = 0;

    // Clear the screen message queue
    virtual void clearScreenMessages() = 0;
};