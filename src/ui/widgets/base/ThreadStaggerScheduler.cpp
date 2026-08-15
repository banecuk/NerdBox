#include "ThreadStaggerScheduler.h"

#include <algorithm>
#include <cstdlib>
#include <numeric>

void ThreadStaggerScheduler::configure(uint8_t barCount, float windowFraction,
                                       uint32_t fallbackPeriodMs, uint32_t minPeriodMs,
                                       uint32_t maxPeriodMs) {
    barCount_ = barCount;
    windowFraction_ = windowFraction;
    fallbackPeriodMs_ = fallbackPeriodMs;
    minPeriodMs_ = minPeriodMs;
    maxPeriodMs_ = maxPeriodMs;

    hasStamp_ = false;
    hasPeriod_ = false;
    periodMs_ = 0;
    windowMs_ = 0;
    lastPattern_ = kPatternCount;

    ranks_.assign(barCount_, 0);
    releaseAtMs_.assign(barCount_, 0);
}

void ThreadStaggerScheduler::seed(uint32_t seedValue) {
    // xorshift32 needs a non-zero state.
    rngState_ = seedValue != 0 ? seedValue : 1;
}

uint32_t ThreadStaggerScheduler::nextRandom() {
    uint32_t x = rngState_;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    rngState_ = x;
    return x;
}

uint32_t ThreadStaggerScheduler::periodMs() const {
    return hasPeriod_ ? periodMs_ : fallbackPeriodMs_;
}

void ThreadStaggerScheduler::tick(uint32_t nowMs, uint32_t publishStampMs) {
    if (!hasStamp_) {
        hasStamp_ = true;
        lastStamp_ = publishStampMs;
        beginEpoch(nowMs);
        return;
    }

    if (publishStampMs == lastStamp_) {
        return;  // no new payload since the last tick
    }

    uint32_t delta = nowMs - lastEpochMs_;
    delta = std::max(minPeriodMs_, std::min(maxPeriodMs_, delta));
    periodMs_ = hasPeriod_ ? static_cast<uint32_t>(periodMs_ * 0.75f + delta * 0.25f) : delta;
    hasPeriod_ = true;

    lastStamp_ = publishStampMs;
    beginEpoch(nowMs);
}

void ThreadStaggerScheduler::beginEpoch(uint32_t nowMs) {
    lastEpochMs_ = nowMs;
    assignRanks();

    windowMs_ = static_cast<uint32_t>(periodMs() * windowFraction_);
    const uint8_t n = barCount_;
    for (uint8_t i = 0; i < n; ++i) {
        releaseAtMs_[i] = nowMs + (static_cast<uint32_t>(ranks_[i]) * windowMs_) / n;
    }
}

void ThreadStaggerScheduler::assignRanks() {
    const uint8_t n = barCount_;
    if (n == 0) {
        return;
    }

    uint8_t pattern = static_cast<uint8_t>(nextRandom() % kPatternCount);
    if (pattern == lastPattern_) {
        pattern = static_cast<uint8_t>((pattern + 1) % kPatternCount);
    }
    lastPattern_ = pattern;

    switch (pattern) {
        case 0:  // SweepRight
            for (uint8_t i = 0; i < n; ++i) {
                ranks_[i] = i;
            }
            break;
        case 1:  // SweepLeft
            for (uint8_t i = 0; i < n; ++i) {
                ranks_[i] = static_cast<uint8_t>(n - 1 - i);
            }
            break;
        case 2:    // CenterOut
        case 3: {  // EdgesIn
            std::vector<uint8_t> order(n);
            std::iota(order.begin(), order.end(), 0);
            std::stable_sort(order.begin(), order.end(), [n](uint8_t a, uint8_t b) {
                const int keyA = std::abs(2 * a - (n - 1));
                const int keyB = std::abs(2 * b - (n - 1));
                return keyA < keyB;
            });
            for (uint8_t rank = 0; rank < n; ++rank) {
                const uint8_t index = order[rank];
                ranks_[index] = (pattern == 2) ? rank : static_cast<uint8_t>(n - 1 - rank);
            }
            break;
        }
        case 4:  // Comb: evens first, then odds
            for (uint8_t i = 0; i < n; ++i) {
                ranks_[i] = (i % 2 == 0) ? static_cast<uint8_t>(i / 2)
                                         : static_cast<uint8_t>((n + 1) / 2 + i / 2);
            }
            break;
        case 5: {  // Shuffle: Fisher-Yates permutation
            std::vector<uint8_t> perm(n);
            std::iota(perm.begin(), perm.end(), 0);
            for (uint8_t i = static_cast<uint8_t>(n - 1); i > 0; --i) {
                const uint8_t j = static_cast<uint8_t>(nextRandom() % (i + 1));
                std::swap(perm[i], perm[j]);
            }
            for (uint8_t rank = 0; rank < n; ++rank) {
                ranks_[perm[rank]] = rank;
            }
            break;
        }
        default:
            break;
    }
}

void ThreadStaggerScheduler::releaseAll(uint32_t nowMs) {
    std::fill(releaseAtMs_.begin(), releaseAtMs_.end(), nowMs);
}

bool ThreadStaggerScheduler::isReleased(uint8_t index, uint32_t nowMs) const {
    return static_cast<int32_t>(nowMs - releaseAtMs_[index]) >= 0;
}
