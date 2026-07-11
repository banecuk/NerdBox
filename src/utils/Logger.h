#pragma once

#include <memory>
#include <string>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include "LoggerInterface.h"

class Logger : public LoggerInterface {
 public:
    Logger(const bool& isTimeSynced);
    ~Logger();

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    // Basic log methods
    void debug(const char* message, bool forScreen = false) override;
    void info(const char* message, bool forScreen = false) override;
    void warning(const char* message, bool forScreen = false) override;
    void error(const char* message, bool forScreen = false) override;
    void critical(const char* message, bool forScreen = false) override;

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

    bool popScreenMessage(char* buffer, size_t bufferSize) override;
    void clearScreenMessages() override;

    size_t copyRecentLogs(LogEntry* outEntries, size_t maxCount) override;

 private:
    const bool& isTimeSynced_;

    // Constants for memory management
    static constexpr size_t MAX_SCREEN_QUEUE_SIZE = 25;  // Prevent memory exhaustion

    // Heap-allocated as its own block (not an inline std::array member) so
    // this ~5.5 KB buffer doesn't inflate the size of whatever aggregate
    // Logger is embedded in — ApplicationComponents also holds the LGFX
    // display object, and growing that single allocation risks tipping it
    // over into PSRAM, where the display's DMA/SPI transfers can't safely
    // run from.
    std::unique_ptr<LogEntry[]> screenQueue_;
    size_t screenQueueHead_ = 0;
    size_t screenQueueCount_ = 0;
    SemaphoreHandle_t screenQueueMutex_ = xSemaphoreCreateMutex();

    // Second, independent ring buffer: every level >= INFO regardless of
    // forScreen, non-destructively read via copyRecentLogs(). Separate from
    // screenQueue_ (which is destructively popped by the boot screen and only
    // ever holds forScreen==true entries) so /logs can see the full recent
    // history, including boot-time messages the boot screen has already
    // consumed.
    std::unique_ptr<LogEntry[]> recentLogs_;
    size_t recentLogsHead_ = 0;
    size_t recentLogsCount_ = 0;
    SemaphoreHandle_t recentLogsMutex_ = xSemaphoreCreateMutex();

    // Buffer-based methods
    void getTimestamp(char* buffer, size_t bufferSize, bool forScreen = false);
    void getUptimeTimestamp(char* buffer, size_t bufferSize, bool forScreen);
    const char* levelToString(LogLevel level);

    // Optimized logging methods
    void logMessage(LogLevel level, const char* message, bool forScreen);
    void logFormatted(LogLevel level, const char* format, va_list args, bool forScreen);
    void pushScreenEntry(const char* timestamp, LogLevel level, const char* message);
    void pushRecentLog(const char* timestamp, LogLevel level, const char* message);
};