#pragma once

#include "LoggerInterface.h"
#include "LogRing.h"
#include "RecentLogView.h"
#include "ScreenLogQueue.h"

class Logger : public LoggerInterface, public ScreenLogQueue, public RecentLogView {
 public:
    Logger(const bool& isTimeSynced);
    ~Logger() = default;

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    // Basic log methods
    void debug(const char* message, bool forScreen = false) override;
    void info(const char* message, bool forScreen = false) override;
    void warning(const char* message, bool forScreen = false) override;
    void error(const char* message, bool forScreen = false) override;
    void critical(const char* message, bool forScreen = false) override;

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

    static constexpr size_t kScreenQueueCapacity = 25;  // Prevent memory exhaustion

    LogRing<kScreenQueueCapacity> screenQueue_;
    LogRing<RecentLogView::kRecentLogCapacity> recentLogs_;

    // Buffer-based methods
    void getTimestamp(char* buffer, size_t bufferSize, bool forScreen = false);
    void getUptimeTimestamp(char* buffer, size_t bufferSize, bool forScreen);
    const char* levelToString(LogLevel level);

    // Single entry point for every log call: the `f`-suffix methods
    // vsnprintf() into a stack buffer first, then call this with the result.
    void emit(LogLevel level, const char* message, bool forScreen);
};
