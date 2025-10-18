#pragma once

#include <Arduino.h>

#include <queue>

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
        String timestamp;
        LogLevel level;
        String message;
        bool forScreen;
    };

    virtual ~LoggerInterface() = default;

    // Basic log methods
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

    // Get all screen-display messages
    virtual std::queue<String> getScreenMessages() = 0;

    // Clear the screen message queue
    virtual void clearScreenMessages() = 0;
};