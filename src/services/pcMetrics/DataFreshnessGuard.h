#pragma once

#include <Arduino.h>

#include "PcMetrics.h"

/**
 * Answers "is the most recent fetch still fresh?" without coupling callers to
 * the two-field check (is_available + timestamp age) that defines freshness.
 *
 * Constructed with a reference to the shared PcMetrics data and a configurable
 * timeout.  All staleness logic lives here; widgets and other consumers only
 * call isFresh().
 *
 * Deliberately non-owning and header-only — it holds a const reference and
 * introduces no heap allocation or RTOS dependency.
 */
class DataFreshnessGuard {
 public:
    static constexpr unsigned long kDefaultTimeoutMs = 5000;

    explicit DataFreshnessGuard(const PcMetrics& metrics,
                                unsigned long timeoutMs = kDefaultTimeoutMs)
        : metrics_(metrics), timeoutMs_(timeoutMs) {}

    // Returns true when data has arrived and the last successful fetch is
    // within the configured timeout window.
    bool isFresh() const {
        if (!metrics_.is_available) {
            return false;
        }
        return (millis() - metrics_.last_update_timestamp) <= timeoutMs_;
    }

    void setTimeout(unsigned long timeoutMs) { timeoutMs_ = timeoutMs; }
    unsigned long getTimeout() const { return timeoutMs_; }

 private:
    const PcMetrics& metrics_;
    unsigned long timeoutMs_;
};
