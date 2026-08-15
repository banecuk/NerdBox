#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include <cstring>
#include <memory>

#include "LogTypes.h"

// Fixed-capacity ring buffer of LogEntry, mutex-guarded. Backs both of
// Logger's ring buffers (the destructively-popped screen queue and the
// non-destructive recent-log view) — identical head/count/modulo bookkeeping,
// used two different ways.
//
// The buffer is heap-allocated rather than an inline array member: Logger is
// a direct member of ApplicationComponents, and growing that aggregate's size
// has corrupted the LCD on real hardware before (see the
// applicationcomponents-size-sensitive note) even with a clean build. Keep it
// that way — do not switch entries_ to a fixed-size array member.
template <size_t N>
class LogRing {
 public:
    LogRing() : entries_(new LogEntry[N]()), mutex_(xSemaphoreCreateMutex()) {}
    ~LogRing() { vSemaphoreDelete(mutex_); }

    LogRing(const LogRing&) = delete;
    LogRing& operator=(const LogRing&) = delete;

    void push(const char* timestamp, LogLevel level, const char* message, bool forScreen) {
        LogEntry entry{};
        strncpy(entry.timestamp, timestamp, sizeof(entry.timestamp) - 1);
        entry.level = level;
        strncpy(entry.message, message, sizeof(entry.message) - 1);
        entry.forScreen = forScreen;

        xSemaphoreTake(mutex_, portMAX_DELAY);
        size_t index = (head_ + count_) % N;
        entries_[index] = entry;
        if (count_ < N) {
            ++count_;
        } else {
            head_ = (head_ + 1) % N;
        }
        xSemaphoreGive(mutex_);
    }

    // Destructively pops the oldest entry's message into buffer. Returns
    // false if the ring is empty.
    bool pop(char* buffer, size_t bufferSize) {
        bool hasMessage = false;
        xSemaphoreTake(mutex_, portMAX_DELAY);
        if (count_ > 0) {
            strncpy(buffer, entries_[head_].message, bufferSize - 1);
            buffer[bufferSize - 1] = '\0';
            head_ = (head_ + 1) % N;
            --count_;
            hasMessage = true;
        }
        xSemaphoreGive(mutex_);
        return hasMessage;
    }

    void clear() {
        xSemaphoreTake(mutex_, portMAX_DELAY);
        head_ = 0;
        count_ = 0;
        xSemaphoreGive(mutex_);
    }

    // Non-destructively copies up to maxCount of the most recent entries into
    // out, oldest-first. Returns the number of entries copied.
    size_t copyRecent(LogEntry* out, size_t maxCount) {
        xSemaphoreTake(mutex_, portMAX_DELAY);
        size_t count = count_ < maxCount ? count_ : maxCount;
        // If fewer are requested than stored, keep the most recent `count` —
        // skip over the oldest (count_ - count) entries.
        size_t start = (head_ + (count_ - count)) % N;
        for (size_t i = 0; i < count; ++i) {
            out[i] = entries_[(start + i) % N];
        }
        xSemaphoreGive(mutex_);
        return count;
    }

 private:
    std::unique_ptr<LogEntry[]> entries_;
    size_t head_ = 0;
    size_t count_ = 0;
    SemaphoreHandle_t mutex_;
};
