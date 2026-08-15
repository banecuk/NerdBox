#pragma once

// Shared data types for the logging subsystem. Split out of LoggerInterface
// so RecentLogView (and any other consumer of LogEntry) doesn't have to pull
// in the full sink interface just to know the shape of a log line.

enum class LogLevel {
    DEBUG,    // For detailed debugging information
    INFO,     // General information
    WARNING,  // Potential issues
    ERROR,    // Error conditions
    CRITICAL  // Critical failures
};

struct LogEntry {
    char timestamp[12];  // "HH:MM:SS" + null — 9 chars, 12 is comfortable
    LogLevel level;
    char message[200];  // matches the screenBuffer size in Logger.cpp
    bool forScreen;
};
