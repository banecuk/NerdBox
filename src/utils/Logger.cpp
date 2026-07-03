#include "Logger.h"

#include <cstring>

// DEBUG_MODE is defined by the build environment (-DDEBUG_MODE=1 or =0).
// Default to 0 (off) so IntelliSense and any translation unit that omits the
// flag both compile cleanly without emitting debug output.
#ifndef DEBUG_MODE
#define DEBUG_MODE 0
#endif

Logger::Logger(const bool& isTimeSynced) : isTimeSynced_(isTimeSynced) {
    // Serial is initialized by main.cpp using the configured baud rate
}

Logger::~Logger() {
    vSemaphoreDelete(screenQueueMutex_);
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
        if (!getLocalTime(&timeinfo, 0)) {
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

// OPTIMIZED: Single buffer approach for all logging
void Logger::logMessage(LogLevel level, const String& message, bool forScreen) {
    if (level == LogLevel::DEBUG && !DEBUG_MODE) {
        return;
    }

    // Single buffer for entire log entry - optimized size
    char logBuffer[256];  // Reduced from 384 - ample for most messages
    char timestamp[24];

    // Build complete log entry in one operation
    getTimestamp(timestamp, sizeof(timestamp), false);
    const char* levelStr = levelToString(level);

    // Direct format to avoid intermediate String operations
    snprintf(logBuffer, sizeof(logBuffer), "%s [%s] %s", timestamp, levelStr, message.c_str());

    // Send to Serial
    Serial.println(logBuffer);

    // For screen messages, use efficient char array approach
    if (forScreen) {
        char screenBuffer[200];  // Separate buffer for screen messages
        char screenTimestamp[24];

        getTimestamp(screenTimestamp, sizeof(screenTimestamp), true);
        const char* shortLevelStr = levelToString(level);

        // Truncate message if too long for screen display
        size_t maxMessageLen = sizeof(screenBuffer) - 32;  // Reserve space for timestamp and level
        const char* messageStr = message.c_str();
        size_t messageLen = strlen(messageStr);
        if (messageLen > maxMessageLen) {
            messageLen = maxMessageLen;
        }

        // Build screen message efficiently
        snprintf(screenBuffer, sizeof(screenBuffer), "[%s] [%s] ", screenTimestamp, shortLevelStr);
        size_t prefixLen = strlen(screenBuffer);
        strncpy(screenBuffer + prefixLen, messageStr, sizeof(screenBuffer) - prefixLen - 1);
        screenBuffer[sizeof(screenBuffer) - 1] = '\0';

        // Store in queue — no heap allocation, copy into fixed char arrays
        LogEntry entry{};
        strncpy(entry.timestamp, screenTimestamp, sizeof(entry.timestamp) - 1);
        entry.level = level;
        strncpy(entry.message, screenBuffer, sizeof(entry.message) - 1);
        entry.forScreen = true;

        xSemaphoreTake(screenQueueMutex_, portMAX_DELAY);
        screenQueue_.push(entry);
        // Limit queue size to prevent memory exhaustion
        while (screenQueue_.size() > MAX_SCREEN_QUEUE_SIZE) {
            screenQueue_.pop();
        }
        xSemaphoreGive(screenQueueMutex_);
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

// OPTIMIZED: Single-pass formatted logging
void Logger::logFormatted(LogLevel level, const char* format, va_list args, bool forScreen) {
    if (level == LogLevel::DEBUG && !DEBUG_MODE) {
        return;
    }

    // Single buffer for entire operation
    char completeBuffer[256];
    char timestamp[24];
    char messageBuffer[192];  // Just for the formatted message part

    // Format the message part first
    vsnprintf(messageBuffer, sizeof(messageBuffer), format, args);

    // Now build complete log entry
    getTimestamp(timestamp, sizeof(timestamp), false);
    const char* levelStr = levelToString(level);

    snprintf(completeBuffer, sizeof(completeBuffer), "%s [%s] %s", timestamp, levelStr,
             messageBuffer);

    // Send to Serial
    Serial.println(completeBuffer);

    // Screen handling with efficient buffer usage
    if (forScreen) {
        char screenBuffer[200];
        char screenTimestamp[24];

        getTimestamp(screenTimestamp, sizeof(screenTimestamp), true);
        const char* shortLevelStr = levelToString(level);

        // Build screen message directly
        snprintf(screenBuffer, sizeof(screenBuffer), "[%s] [%s] %s", screenTimestamp, shortLevelStr,
                 messageBuffer);

        // Store in queue — no heap allocation
        LogEntry entry{};
        strncpy(entry.timestamp, screenTimestamp, sizeof(entry.timestamp) - 1);
        entry.level = level;
        strncpy(entry.message, screenBuffer, sizeof(entry.message) - 1);
        entry.forScreen = true;

        xSemaphoreTake(screenQueueMutex_, portMAX_DELAY);
        screenQueue_.push(entry);
        // Limit queue size
        while (screenQueue_.size() > MAX_SCREEN_QUEUE_SIZE) {
            screenQueue_.pop();
        }
        xSemaphoreGive(screenQueueMutex_);
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

    xSemaphoreTake(screenQueueMutex_, portMAX_DELAY);
    while (!screenQueue_.empty()) {
        const LogEntry& entry = screenQueue_.front();
        result.push(String(entry.message));
        screenQueue_.pop();
    }
    xSemaphoreGive(screenQueueMutex_);

    return result;
}

void Logger::clearScreenMessages() {
    xSemaphoreTake(screenQueueMutex_, portMAX_DELAY);
    while (!screenQueue_.empty()) {
        screenQueue_.pop();
    }
    xSemaphoreGive(screenQueueMutex_);
}