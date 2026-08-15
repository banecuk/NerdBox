#pragma once

#include <cstddef>

#include "LogTypes.h"

// The non-destructive recent-log ring: level >= INFO, independent of
// forScreen/DEBUG_MODE. Split out of LoggerInterface so consumers that only
// want post-mortem inspection (e.g. GET /logs) don't have to depend on the
// full log-sink interface.
class RecentLogView {
 public:
    virtual ~RecentLogView() = default;

    // Capacity of the ring buffer — callers size their copy buffer off this
    // constant instead of a magic number.
    static constexpr size_t kRecentLogCapacity = 50;

    // Non-destructively copies up to maxCount of the most recent log entries
    // into outEntries, oldest-first. Returns the number of entries copied.
    // Unlike ScreenLogQueue::popScreenMessage(), repeated calls see the same
    // entries.
    virtual size_t copyRecentLogs(LogEntry* outEntries, size_t maxCount) = 0;
};
