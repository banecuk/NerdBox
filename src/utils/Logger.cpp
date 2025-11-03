#include "Logger.h"

Logger::Logger(const bool& isTimeSynced) : isTimeSynced_(isTimeSynced) {
    // Initialize Serial if needed
    Serial.begin(115200);
}

Logger::~Logger() {
    // Cleanup if needed
}

void Logger::getUptimeTimestamp(char* buffer, size_t bufferSize, bool forScreen) {
    unsigned long ms = millis();
    unsigned long seconds = ms / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;

    if (forScreen) {
        snprintf(buffer, bufferSize, "%02lu:%02lu:%02lu", hours, minutes % 60, seconds % 60);
    } else {
        snprintf(buffer, bufferSize, "%02lu:%02lu:%02lu.%03lu", hours, minutes % 60, seconds % 60,
                 ms % 1000);
    }
}

void Logger::getTimestamp(char* buffer, size_t bufferSize, bool forScreen) {
    if (!isTimeSynced_) {
        getUptimeTimestamp(buffer, bufferSize, forScreen);
    } else {
        struct tm timeinfo;
        if (!getLocalTime(&timeinfo)) {
            getUptimeTimestamp(buffer, bufferSize, forScreen);
        } else {
            if (forScreen) {
                snprintf(buffer, bufferSize, "%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min,
                         timeinfo.tm_sec);
            } else {
                snprintf(buffer, bufferSize, "%02d:%02d:%02d.%03d", timeinfo.tm_hour,
                         timeinfo.tm_min, timeinfo.tm_sec, (int)(millis() % 1000));
            }
        }
    }
}

const char* Logger::levelToString(LogLevel level) {
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
    if (level == LogLevel::DEBUG && !DEBUG_MODE) {
        return;
    }

    // Use a single stack buffer for the entire log entry
    char logBuffer[384];  // Timestamp(20) + Level(10) + Message(200) + formatting(4) + margin
    char timestamp[24];

    getTimestamp(timestamp, sizeof(timestamp), false);
    const char* levelStr = levelToString(level);

    // Build log entry in single buffer
    snprintf(logBuffer, sizeof(logBuffer), "%s [%s] %s", timestamp, levelStr, message.c_str());

    // Always send to Serial
    Serial.println(logBuffer);

    // If marked for screen, add to queue
    if (forScreen) {
        char screenTimestamp[24];
        getTimestamp(screenTimestamp, sizeof(screenTimestamp), true);

        // Limit message length for screen
        String limitedMessage = message.substring(0, 200);
        LogEntry entry{String(screenTimestamp), level, limitedMessage, true};
        screenQueue_.push(entry);
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

void Logger::logFormatted(LogLevel level, const char* format, va_list args, bool forScreen) {
    char messageBuffer[256];  // Stack buffer for formatted messages
    vsnprintf(messageBuffer, sizeof(messageBuffer), format, args);

    if (level == LogLevel::DEBUG && !DEBUG_MODE) {
        return;
    }

    // Build log entry directly in buffer
    char logBuffer[384];
    char timestamp[24];

    getTimestamp(timestamp, sizeof(timestamp), false);
    const char* levelStr = levelToString(level);

    snprintf(logBuffer, sizeof(logBuffer), "%s [%s] %s", timestamp, levelStr, messageBuffer);

    // Send to Serial
    Serial.println(logBuffer);

    // Screen queue still needs String for compatibility
    if (forScreen) {
        char screenTimestamp[24];
        getTimestamp(screenTimestamp, sizeof(screenTimestamp), true);
        LogEntry entry{String(screenTimestamp), level, String(messageBuffer), true};
        screenQueue_.push(entry);
    }
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

        char buffer[256];
        const char* levelStr = levelToString(entry.level);
        snprintf(buffer, sizeof(buffer), "[%s] [%s] %s", entry.timestamp.c_str(), levelStr,
                 entry.message.c_str());

        result.push(String(buffer));
        screenQueue_.pop();
    }

    return result;
}

void Logger::clearScreenMessages() {
    while (!screenQueue_.empty()) {
        screenQueue_.pop();
    }
}