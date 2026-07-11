#pragma once

#include <Arduino.h>

// Answers "is the most recent fetch still fresh?" without coupling callers to
// the two-field check (availability + timestamp age) that defines freshness —
// shared by PcMetrics and AirQualityData, which use this field pair under
// different names and types: PcMetrics's is_available is std::atomic<bool>
// (published cross-core from the background task), AirQualityData's is a
// plain bool. Templated so both bind directly instead of forcing one of them
// through a lossy conversion.
//
// isFresh() reads isAvailable_ before lastUpdateMs_, in that order — when
// isAvailable_ is atomic, observing it true synchronizes-with the writer's
// release of that flag, which happens-before every field the writer set
// prior to it (including the timestamp), so the subsequent plain read of
// lastUpdateMs_ is guaranteed to see the matching value, not a torn/stale one.
//
// Deliberately non-owning and header-only — holds references, no heap
// allocation or RTOS dependency.
template <typename AvailabilityT, typename TimestampT>
class DataFreshnessGuard {
 public:
    static constexpr unsigned long kDefaultTimeoutMs = 5000;

    DataFreshnessGuard(const AvailabilityT& isAvailable, const TimestampT& lastUpdateMs,
                       unsigned long timeoutMs = kDefaultTimeoutMs)
        : isAvailable_(isAvailable), lastUpdateMs_(lastUpdateMs), timeoutMs_(timeoutMs) {}

    bool isFresh() const {
        if (!isAvailable_) {
            return false;
        }
        return (millis() - static_cast<unsigned long>(lastUpdateMs_)) <= timeoutMs_;
    }

    void setTimeout(unsigned long timeoutMs) { timeoutMs_ = timeoutMs; }
    unsigned long getTimeout() const { return timeoutMs_; }

 private:
    const AvailabilityT& isAvailable_;
    const TimestampT& lastUpdateMs_;
    unsigned long timeoutMs_;
};
