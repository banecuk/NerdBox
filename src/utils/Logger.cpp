#include "Logger.h"

#include <cstring>

// DEBUG_MODE is defined by the build environment (-DDEBUG_MODE=1 or =0).
// Default to 0 (off) so IntelliSense and any translation unit that omits the
// flag both compile cleanly without emitting debug output.
#ifndef DEBUG_MODE
    #define DEBUG_MODE 0
#endif

Logger::Logger(const bool& isTimeSynced)
    : isTimeSynced_(isTimeSynced),
      screenQueue_(new LogEntry[MAX_SCREEN_QUEUE_SIZE]()),
      recentLogs_(new LogEntry[kRecentLogCapacity]()) {
    // Serial is initialized by main.cpp using the configured baud rate
}

Logger::~Logger() {
    vSemaphoreDelete(screenQueueMutex_);
    vSemaphoreDelete(recentLogsMutex_);
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
void Logger::logMessage(LogLevel level, const char* message, bool forScreen) {
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
    snprintf(logBuffer, sizeof(logBuffer), "%s [%s] %s", timestamp, levelStr, message);

    // Send to Serial
    Serial.println(logBuffer);

    // The recent-log ring and the screen queue both want the short
    // "HH:MM:SS" timestamp (forScreen=true) — compute it at most once per
    // call instead of once per consumer; getTimestamp() re-derives from
    // getLocalTime() each time it's called.
    char shortTimestamp[12];
    if (level != LogLevel::DEBUG || forScreen) {
        getTimestamp(shortTimestamp, sizeof(shortTimestamp), true);
    }

    if (level != LogLevel::DEBUG) {
        pushRecentLog(shortTimestamp, level, message);
    }

    // For screen messages, use efficient char array approach
    if (forScreen) {
        char screenBuffer[200];  // Separate buffer for screen messages

        // Truncate message if too long for screen display
        size_t maxMessageLen = sizeof(screenBuffer) - 32;  // Reserve space for timestamp and level
        size_t messageLen = strlen(message);
        if (messageLen > maxMessageLen) {
            messageLen = maxMessageLen;
        }

        // Build screen message efficiently
        snprintf(screenBuffer, sizeof(screenBuffer), "[%s] [%s] ", shortTimestamp, levelStr);
        size_t prefixLen = strlen(screenBuffer);
        strncpy(screenBuffer + prefixLen, message, sizeof(screenBuffer) - prefixLen - 1);
        screenBuffer[sizeof(screenBuffer) - 1] = '\0';

        pushScreenEntry(shortTimestamp, level, screenBuffer);
    }
}

void Logger::debug(const char* message, bool forScreen) {
    logMessage(LogLevel::DEBUG, message, forScreen);
}

void Logger::info(const char* message, bool forScreen) {
    logMessage(LogLevel::INFO, message, forScreen);
}

void Logger::warning(const char* message, bool forScreen) {
    logMessage(LogLevel::WARNING, message, forScreen);
}

void Logger::error(const char* message, bool forScreen) {
    logMessage(LogLevel::ERROR, message, forScreen);
}

void Logger::critical(const char* message, bool forScreen) {
    logMessage(LogLevel::CRITICAL, message, forScreen);
}

void Logger::debug(const String& message, bool forScreen) {
    logMessage(LogLevel::DEBUG, message.c_str(), forScreen);
}

void Logger::info(const String& message, bool forScreen) {
    logMessage(LogLevel::INFO, message.c_str(), forScreen);
}

void Logger::warning(const String& message, bool forScreen) {
    logMessage(LogLevel::WARNING, message.c_str(), forScreen);
}

void Logger::error(const String& message, bool forScreen) {
    logMessage(LogLevel::ERROR, message.c_str(), forScreen);
}

void Logger::critical(const String& message, bool forScreen) {
    logMessage(LogLevel::CRITICAL, message.c_str(), forScreen);
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

    // Both consumers below want the short "HH:MM:SS" timestamp — compute it
    // at most once per call rather than once per consumer.
    char shortTimestamp[12];
    if (level != LogLevel::DEBUG || forScreen) {
        getTimestamp(shortTimestamp, sizeof(shortTimestamp), true);
    }

    if (level != LogLevel::DEBUG) {
        pushRecentLog(shortTimestamp, level, messageBuffer);
    }

    // Screen handling with efficient buffer usage
    if (forScreen) {
        char screenBuffer[200];

        // Build screen message directly
        snprintf(screenBuffer, sizeof(screenBuffer), "[%s] [%s] %s", shortTimestamp, levelStr,
                 messageBuffer);

        pushScreenEntry(shortTimestamp, level, screenBuffer);
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

void Logger::pushScreenEntry(const char* timestamp, LogLevel level, const char* message) {
    LogEntry entry{};
    strncpy(entry.timestamp, timestamp, sizeof(entry.timestamp) - 1);
    entry.level = level;
    strncpy(entry.message, message, sizeof(entry.message) - 1);
    entry.forScreen = true;

    xSemaphoreTake(screenQueueMutex_, portMAX_DELAY);
    size_t index = (screenQueueHead_ + screenQueueCount_) % MAX_SCREEN_QUEUE_SIZE;
    screenQueue_[index] = entry;
    if (screenQueueCount_ < MAX_SCREEN_QUEUE_SIZE) {
        ++screenQueueCount_;
    } else {
        screenQueueHead_ = (screenQueueHead_ + 1) % MAX_SCREEN_QUEUE_SIZE;
    }
    xSemaphoreGive(screenQueueMutex_);
}

bool Logger::popScreenMessage(char* buffer, size_t bufferSize) {
    bool hasMessage = false;

    xSemaphoreTake(screenQueueMutex_, portMAX_DELAY);
    if (screenQueueCount_ > 0) {
        const LogEntry& entry = screenQueue_[screenQueueHead_];
        strncpy(buffer, entry.message, bufferSize - 1);
        buffer[bufferSize - 1] = '\0';
        screenQueueHead_ = (screenQueueHead_ + 1) % MAX_SCREEN_QUEUE_SIZE;
        --screenQueueCount_;
        hasMessage = true;
    }
    xSemaphoreGive(screenQueueMutex_);

    return hasMessage;
}

void Logger::clearScreenMessages() {
    xSemaphoreTake(screenQueueMutex_, portMAX_DELAY);
    screenQueueHead_ = 0;
    screenQueueCount_ = 0;
    xSemaphoreGive(screenQueueMutex_);
}

void Logger::pushRecentLog(const char* timestamp, LogLevel level, const char* message) {
    LogEntry entry{};
    strncpy(entry.timestamp, timestamp, sizeof(entry.timestamp) - 1);
    entry.level = level;
    strncpy(entry.message, message, sizeof(entry.message) - 1);
    entry.forScreen = false;

    xSemaphoreTake(recentLogsMutex_, portMAX_DELAY);
    size_t index = (recentLogsHead_ + recentLogsCount_) % kRecentLogCapacity;
    recentLogs_[index] = entry;
    if (recentLogsCount_ < kRecentLogCapacity) {
        ++recentLogsCount_;
    } else {
        recentLogsHead_ = (recentLogsHead_ + 1) % kRecentLogCapacity;
    }
    xSemaphoreGive(recentLogsMutex_);
}

size_t Logger::copyRecentLogs(LogEntry* outEntries, size_t maxCount) {
    xSemaphoreTake(recentLogsMutex_, portMAX_DELAY);
    size_t count = recentLogsCount_ < maxCount ? recentLogsCount_ : maxCount;
    // If fewer are requested than stored, keep the most recent `count` —
    // skip over the oldest (recentLogsCount_ - count) entries.
    size_t start = (recentLogsHead_ + (recentLogsCount_ - count)) % kRecentLogCapacity;
    for (size_t i = 0; i < count; ++i) {
        outEntries[i] = recentLogs_[(start + i) % kRecentLogCapacity];
    }
    xSemaphoreGive(recentLogsMutex_);
    return count;
}