#pragma once

#include <Arduino.h>

#include "utils/PublishedFlag.h"

// Answers "is the most recent fetch still fresh?" without coupling callers to
// the two-field check (availability + timestamp age) that defines freshness.
// Binds directly to a PublishedFlag — every data source that publishes
// cross-core (PcMetrics, AirQualityData, WeatherData) owns one, so there is
// exactly one freshness contract shared by all of them.
//
// isFresh() reads PublishedFlag::available() (an acquire load) before
// lastUpdateMs() (a plain read) — observing available() true synchronizes-
// with the writer's release in publish(), which happens-before every field
// the writer set prior to it (including the timestamp), so the subsequent
// plain read of lastUpdateMs() is guaranteed to see the matching value, not
// a torn/stale one.
//
// Deliberately non-owning and header-only — holds a reference, no heap
// allocation or RTOS dependency.
class DataFreshnessGuard {
 public:
    static constexpr unsigned long kDefaultTimeoutMs = 5000;

    // Defaults to the real clock; tests inject a fake one via the third
    // constructor argument to control elapsed time deterministically without
    // touching any production call site.
    using ClockFn = unsigned long (*)();

    explicit DataFreshnessGuard(const PublishedFlag& published,
                                unsigned long timeoutMs = kDefaultTimeoutMs,
                                ClockFn clock = &millis)
        : published_(published), timeoutMs_(timeoutMs), clock_(clock) {}

    bool isFresh() const {
        if (!published_.available()) {
            return false;
        }
        return (clock_() - published_.lastUpdateMs()) <= timeoutMs_;
    }

    void setTimeout(unsigned long timeoutMs) { timeoutMs_ = timeoutMs; }
    unsigned long getTimeout() const { return timeoutMs_; }

 private:
    const PublishedFlag& published_;
    unsigned long timeoutMs_;
    ClockFn clock_;
};
