#include "Logger.h"

#include <cstdarg>
#include <cstring>

#include <Arduino.h>

// DEBUG_MODE is defined by the build environment (-DDEBUG_MODE=1 or =0).
// Default to 0 (off) so IntelliSense and any translation unit that omits the
// flag both compile cleanly without emitting debug output.
#ifndef DEBUG_MODE
    #define DEBUG_MODE 0
#endif

Logger::Logger(const bool& isTimeSynced) : isTimeSynced_(isTimeSynced) {
    // Serial is initialized by main.cpp using the configured baud rate
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

// Single entry point for both the plain and formatted log methods below —
// same timestamp handling, same short-timestamp optimisation, same
// screen-buffer assembly, previously written out twice as logMessage()/
// logFormatted() differing only in whether the message arrived pre-formatted.
void Logger::emit(LogLevel level, const char* message, bool forScreen) {
    if (level == LogLevel::DEBUG && !DEBUG_MODE) {
        return;
    }

    // Single buffer for entire log entry - optimized size
    char logBuffer[256];
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
        recentLogs_.push(shortTimestamp, level, message, false);
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

        screenQueue_.push(shortTimestamp, level, screenBuffer, true);
    }
}

void Logger::debug(const char* message, bool forScreen) {
    emit(LogLevel::DEBUG, message, forScreen);
}

void Logger::info(const char* message, bool forScreen) {
    emit(LogLevel::INFO, message, forScreen);
}

void Logger::warning(const char* message, bool forScreen) {
    emit(LogLevel::WARNING, message, forScreen);
}

void Logger::error(const char* message, bool forScreen) {
    emit(LogLevel::ERROR, message, forScreen);
}

void Logger::critical(const char* message, bool forScreen) {
    emit(LogLevel::CRITICAL, message, forScreen);
}

void Logger::debugf(const char* format, ...) {
    char messageBuffer[192];
    va_list args;
    va_start(args, format);
    vsnprintf(messageBuffer, sizeof(messageBuffer), format, args);
    va_end(args);
    emit(LogLevel::DEBUG, messageBuffer, false);
}

void Logger::infof(const char* format, ...) {
    char messageBuffer[192];
    va_list args;
    va_start(args, format);
    vsnprintf(messageBuffer, sizeof(messageBuffer), format, args);
    va_end(args);
    emit(LogLevel::INFO, messageBuffer, false);
}

void Logger::warningf(const char* format, ...) {
    char messageBuffer[192];
    va_list args;
    va_start(args, format);
    vsnprintf(messageBuffer, sizeof(messageBuffer), format, args);
    va_end(args);
    emit(LogLevel::WARNING, messageBuffer, false);
}

void Logger::errorf(const char* format, ...) {
    char messageBuffer[192];
    va_list args;
    va_start(args, format);
    vsnprintf(messageBuffer, sizeof(messageBuffer), format, args);
    va_end(args);
    emit(LogLevel::ERROR, messageBuffer, false);
}

void Logger::criticalf(const char* format, ...) {
    char messageBuffer[192];
    va_list args;
    va_start(args, format);
    vsnprintf(messageBuffer, sizeof(messageBuffer), format, args);
    va_end(args);
    emit(LogLevel::CRITICAL, messageBuffer, false);
}

bool Logger::popScreenMessage(char* buffer, size_t bufferSize) {
    return screenQueue_.pop(buffer, bufferSize);
}

void Logger::clearScreenMessages() {
    screenQueue_.clear();
}

size_t Logger::copyRecentLogs(LogEntry* outEntries, size_t maxCount) {
    return recentLogs_.copyRecent(outEntries, maxCount);
}
