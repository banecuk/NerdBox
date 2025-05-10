#ifndef LOGGER_H
#define LOGGER_H

#include "ILogger.h"

class Logger : public ILogger {
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

    String getTimestamp(bool forScreen = false);
    String getUptimeTimestamp(bool forScreen);
    String levelToString(LogLevel level);
    void logMessage(LogLevel level, const String& message, bool forScreen);
    void logFormatted(LogLevel level, const char* format, va_list args, bool forScreen);
};

#endif