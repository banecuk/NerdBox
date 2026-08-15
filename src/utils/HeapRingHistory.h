#pragma once

#include <esp_heap_caps.h>

#include <cstddef>

// Ring buffer of history samples backed by a heap allocation instead of a
// compile-time member array — for histories whose capacity is only known at
// runtime (derived from a widget's actual on-screen width). Same access
// pattern as RingHistory (at(0) is oldest, at(size()-1) is newest), but
// heap-allocated and non-copyable since it owns that allocation.
//
// Allocated with MALLOC_CAP_8BIT (internal RAM, falling back to PSRAM only
// if internal RAM is exhausted) rather than forcing MALLOC_CAP_SPIRAM — at
// the size these histories actually run (well under 1 KB total across all
// four sparklines), PSRAM's slower cache-miss latency buys nothing and only
// costs an indirection.
template <typename T>
class HeapRingHistory {
 public:
    explicit HeapRingHistory(size_t capacity)
        : capacity_(capacity),
          buf_(static_cast<T*>(heap_caps_malloc(capacity * sizeof(T), MALLOC_CAP_8BIT))) {
        clear();
    }

    ~HeapRingHistory() { heap_caps_free(buf_); }

    HeapRingHistory(const HeapRingHistory&) = delete;
    HeapRingHistory& operator=(const HeapRingHistory&) = delete;

    void push(T value) {
        if (!buf_)
            return;
        buf_[head_] = value;
        head_ = (head_ + 1 == capacity_) ? 0 : head_ + 1;
        if (count_ < capacity_)
            ++count_;
    }

    void clear() {
        head_ = 0;
        count_ = 0;
        if (!buf_)
            return;
        for (size_t i = 0; i < capacity_; ++i)
            buf_[i] = T{};
    }

    size_t size() const { return count_; }
    size_t capacity() const { return capacity_; }

    // Ring index of the oldest retained sample (i.e. at(0)'s slot). Callers
    // that walk several consecutive indices (e.g. i = 0..size()-1) should
    // call this once and index via valueAtOffset() instead of at() per
    // element — at() recomputes this from scratch (a modulo) every call.
    size_t startIndex() const {
        return count_ <= head_ ? head_ - count_ : head_ + capacity_ - count_;
    }

    // Value at ring position (start + i), where start came from startIndex()
    // and 0 <= i < capacity_ (true for any i < size()). Replaces at()'s
    // second modulo with a conditional subtract, since (start + i) can
    // exceed capacity_ by at most capacity_ - 1.
    T valueAtOffset(size_t start, size_t i) const {
        size_t pos = start + i;
        if (pos >= capacity_)
            pos -= capacity_;
        return buf_[pos];
    }

    T at(size_t i) const { return valueAtOffset(startIndex(), i); }

 private:
    size_t capacity_;
    T* buf_;
    size_t head_ = 0;
    size_t count_ = 0;
};
