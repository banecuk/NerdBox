#pragma once

#include <cstddef>

// The boot screen's message queue: destructively popped, forScreen==true
// entries only. Split out of LoggerInterface so BootScreen — the only
// consumer — doesn't have to depend on the full log-sink interface.
class ScreenLogQueue {
 public:
    virtual ~ScreenLogQueue() = default;

    // Pop the oldest queued screen message into `buffer` (truncated to fit,
    // always null-terminated). Returns false if the queue is empty.
    virtual bool popScreenMessage(char* buffer, size_t bufferSize) = 0;

    // Clear the screen message queue.
    virtual void clearScreenMessages() = 0;
};
