#pragma once

#include <Arduino.h>

// Answers "is the most recent fetch still fresh?" without coupling callers to
// the two-field check (availability + timestamp age) that defines freshness —
// shared by PcMetrics and AirQualityData, which use this field pair under
// different names.
//
// Deliberately non-owning and header-only — holds references, no heap
// allocation or RTOS dependency.
class DataFreshnessGuard {
 public:
    static constexpr unsigned long kDefaultTimeoutMs = 5000;

    DataFreshnessGuard(const bool& isAvailable, const unsigned long& lastUpdateMs,
                       unsigned long timeoutMs = kDefaultTimeoutMs)
        : isAvailable_(isAvailable), lastUpdateMs_(lastUpdateMs), timeoutMs_(timeoutMs) {}

    bool isFresh() const {
        if (!isAvailable_) {
            return false;
        }
        return (millis() - lastUpdateMs_) <= timeoutMs_;
    }

    void setTimeout(unsigned long timeoutMs) { timeoutMs_ = timeoutMs; }
    unsigned long getTimeout() const { return timeoutMs_; }

 private:
    const bool& isAvailable_;
    const unsigned long& lastUpdateMs_;
    unsigned long timeoutMs_;
};
