#pragma once

#include <string>

#include "LoggerInterface.h"

class Logger : public LoggerInterface {
 public:
    Logger(const bool& isTimeSynced);
    ~Logger();

    // Basic log methods
    void debug(const String& message, bool forScreen = false) override;
    void info(const String& message, bool forScreen = false) override;
    void warning(const String& message, bool forScreen = false) override;
    void error(const String& message, bool forScreen = false) override;
    void critical(const String& message, bool forScreen = false) override;

    // Formatted log methods
    void debugf(const char* format, ...) override;
    void infof(const char* format, ...) override;
    void warningf(const char* format, ...) override;
    void errorf(const char* format, ...) override;
    void criticalf(const char* format, ...) override;

    std::queue<String> getScreenMessages() override;
    void clearScreenMessages() override;

 private:
    const bool& isTimeSynced_;
    std::queue<LogEntry> screenQueue_;

    // Constants for memory management
    static constexpr size_t MAX_SCREEN_QUEUE_SIZE = 25;  // Prevent memory exhaustion

    // Buffer-based methods
    void getTimestamp(char* buffer, size_t bufferSize, bool forScreen = false);
    void getUptimeTimestamp(char* buffer, size_t bufferSize, bool forScreen);
    const char* levelToString(LogLevel level);

    // Optimized logging methods
    void logMessage(LogLevel level, const String& message, bool forScreen);
    void logFormatted(LogLevel level, const char* format, va_list args, bool forScreen);
};