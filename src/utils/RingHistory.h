#pragma once

#include <cstddef>

// Fixed-capacity ring buffer of history samples — no heap allocation, no
// resizing. Used by GameFpsWidget and LoadHistoryWidget to keep a rolling
// window of recent values for sparkline-style rendering.
//
// at(i) is oldest-first: at(0) is the oldest sample still retained, at(size()-1)
// is the most recently pushed one. Once full, push() overwrites the oldest slot.
template <typename T, size_t N>
class RingHistory {
 public:
    void push(T value) {
        buf_[head_] = value;
        head_ = (head_ + 1) % N;
        if (count_ < N)
            ++count_;
    }

    void clear() {
        head_ = 0;
        count_ = 0;
    }

    size_t size() const { return count_; }
    static constexpr size_t capacity() { return N; }

    T at(size_t i) const {
        const size_t start = (head_ + N - count_) % N;
        return buf_[(start + i) % N];
    }

    T maxValue(T ifEmpty) const {
        if (count_ == 0)
            return ifEmpty;
        T m = at(0);
        for (size_t i = 1; i < count_; ++i) {
            const T v = at(i);
            if (v > m)
                m = v;
        }
        return m;
    }

 private:
    T buf_[N] = {};
    size_t head_ = 0;
    size_t count_ = 0;
};
