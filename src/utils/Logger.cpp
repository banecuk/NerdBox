#include "Logger.h"

Logger::Logger(const bool& isTimeSynced) : isTimeSynced_(isTimeSynced) {
    // Initialize Serial if needed
    Serial.begin(115200);
}

Logger::~Logger() {
    // Cleanup if needed
}

String Logger::getUptimeTimestamp(bool forScreen) {
    char buffer[20];
    unsigned long ms = millis();
    unsigned long seconds = ms / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;

    if (forScreen)
        snprintf(buffer, sizeof(buffer), "%02lu:%02lu:%02lu", hours, minutes % 60,
                 seconds % 60);
    else
        snprintf(buffer, sizeof(buffer), "%02lu:%02lu:%02lu.%03lu", hours, minutes % 60,
                 seconds % 60, ms % 1000);

    return String(buffer);
}

String Logger::getTimestamp(bool forScreen) {
    char buffer[20];
    if (!isTimeSynced_) {
        return getUptimeTimestamp(forScreen);
    } else {
        struct tm timeinfo;
        if (!getLocalTime(&timeinfo)) {
            return getUptimeTimestamp(forScreen);
        } else {
            if (forScreen)
                snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d", timeinfo.tm_hour,
                         timeinfo.tm_min, timeinfo.tm_sec);
            else
                snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d.%03d", timeinfo.tm_hour,
                         timeinfo.tm_min, timeinfo.tm_sec, (int)(millis() % 1000));
            return String(buffer);
        }
    }
}

String Logger::levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG:
            return "DEBUG";
        case LogLevel::INFO:
            return "INFO";
        case LogLevel::WARNING:
            return "WARNING";
        case LogLevel::ERROR:
            return "ERROR";
        case LogLevel::CRITICAL:
            return "CRITICAL";
        default:
            return "UNKNOWN";
    }
}

void Logger::logMessage(LogLevel level, const String& message, bool forScreen) {
    // TODO: if debug mode is not enabled, skip debug messages

    String timestamp = getTimestamp();
    String levelStr = levelToString(level);
    String logEntry = timestamp + " [" + levelStr + "] " + message;

    // Always send to Serial
    Serial.println(logEntry);

    // If marked for screen, add to queue
    if (forScreen) {
        timestamp = getTimestamp(forScreen);
        LogEntry entry{timestamp, level, message, true};  // Create the struct
        screenQueue_.push(entry);  // Push the struct, not the formatted string
    }
}

void Logger::debug(const String& message, bool forScreen) {
    logMessage(LogLevel::DEBUG, message, forScreen);
}

void Logger::info(const String& message, bool forScreen) {
    logMessage(LogLevel::INFO, message, forScreen);
}

void Logger::warning(const String& message, bool forScreen) {
    logMessage(LogLevel::WARNING, message, forScreen);
}

void Logger::error(const String& message, bool forScreen) {
    logMessage(LogLevel::ERROR, message, forScreen);
}

void Logger::critical(const String& message, bool forScreen) {
    logMessage(LogLevel::CRITICAL, message, forScreen);
}

void Logger::logFormatted(LogLevel level, const char* format, va_list args,
                          bool forScreen) {
    char buffer[256];  // Adjust size as needed
    vsnprintf(buffer, sizeof(buffer), format, args);
    logMessage(level, String(buffer), forScreen);
}

void Logger::debugf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    logFormatted(LogLevel::DEBUG, format, args, false);
    va_end(args);
}

void Logger::infof(const char* format, ...) {
    va_list args;
    va_start(args, format);
    logFormatted(LogLevel::INFO, format, args, false);
    va_end(args);
}

void Logger::warningf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    logFormatted(LogLevel::WARNING, format, args, false);
    va_end(args);
}

void Logger::errorf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    logFormatted(LogLevel::ERROR, format, args, false);
    va_end(args);
}

void Logger::criticalf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    logFormatted(LogLevel::CRITICAL, format, args, false);
    va_end(args);
}

std::queue<String> Logger::getScreenMessages() {
    std::queue<String> result;

    while (!screenQueue_.empty()) {
        LogEntry entry = screenQueue_.front();
        String formatted = "[" + entry.timestamp + "] [" + levelToString(entry.level) +
                           "] " + entry.message;
        result.push(formatted);
        screenQueue_.pop();
    }

    return result;
}

void Logger::clearScreenMessages() {
    while (!screenQueue_.empty()) {
        screenQueue_.pop();
    }
}