#pragma once

#include <cstddef>

#include "LogTypes.h"

// The boot screen's message queue: destructively popped, forScreen==true
// entries only. Split out of LoggerInterface so BootScreen — the only
// consumer — doesn't have to depend on the full log-sink interface.
class ScreenLogQueue {
 public:
    virtual ~ScreenLogQueue() = default;

    // Pop the oldest queued screen message into `buffer` (truncated to fit,
    // always null-terminated), and its level into `outLevel` if given.
    // Returns false if the queue is empty. `outLevel` defaults to nullptr so
    // existing callers that only want the text keep compiling unchanged.
    virtual bool popScreenMessage(char* buffer, size_t bufferSize,
                                  LogLevel* outLevel = nullptr) = 0;

    // Clear the screen message queue.
    virtual void clearScreenMessages() = 0;
};
