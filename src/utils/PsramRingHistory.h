#pragma once

#include <esp_heap_caps.h>

#include <cstddef>

// Ring buffer of history samples backed by a PSRAM heap allocation instead
// of a compile-time member array — for histories too long (multi-minute) to
// justify sitting on internal SRAM inline in a widget object. Same access
// pattern as RingHistory (at(0) is oldest, at(size()-1) is newest), but
// heap-allocated and non-copyable since it owns that allocation.
template <typename T>
class PsramRingHistory {
 public:
    explicit PsramRingHistory(size_t capacity)
        : capacity_(capacity),
          buf_(static_cast<T*>(heap_caps_malloc(capacity * sizeof(T), MALLOC_CAP_SPIRAM))) {
        clear();
    }

    ~PsramRingHistory() { heap_caps_free(buf_); }

    PsramRingHistory(const PsramRingHistory&) = delete;
    PsramRingHistory& operator=(const PsramRingHistory&) = delete;

    void push(T value) {
        buf_[head_] = value;
        head_ = (head_ + 1) % capacity_;
        if (count_ < capacity_)
            ++count_;
    }

    void clear() {
        head_ = 0;
        count_ = 0;
        for (size_t i = 0; i < capacity_; ++i)
            buf_[i] = T{};
    }

    size_t size() const { return count_; }
    size_t capacity() const { return capacity_; }

    T at(size_t i) const {
        const size_t start = (head_ + capacity_ - count_) % capacity_;
        return buf_[(start + i) % capacity_];
    }

 private:
    size_t capacity_;
    T* buf_;
    size_t head_ = 0;
    size_t count_ = 0;
};
