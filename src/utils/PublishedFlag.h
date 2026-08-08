#pragma once

#include <Arduino.h>

#include <atomic>

// Shared "cross-core publish" contract for data written by a background/
// network task and read by the screen task: an atomically-published
// availability flag plus the millis() timestamp of the last successful
// write. Every data source that fetches from a remote API in this codebase
// (PcMetrics, AirQualityData, WeatherData) used to reinvent this pair with
// slightly different types and no shared publish path — this is the single
// place that pair now lives.
//
// publish() sets the timestamp before the flag (release order) so a reader
// that observes available() == true is guaranteed — via the happens-before
// edge on the atomic release/acquire pair — to see the matching timestamp
// too, not a stale one. See DataFreshnessGuard for the full argument.
//
// Sticky-on-success by design: every current caller simply skips publish()
// on a failed fetch, leaving the last known-good reading (and its growing
// age) in place rather than explicitly clearing availability. There is no
// markUnavailable() because nothing in this codebase needs it today — add
// one deliberately (not by accident) if a caller ever does.
class PublishedFlag {
 public:
    void publish(unsigned long nowMs) {
        lastUpdateMs_ = nowMs;
        available_.store(true, std::memory_order_release);
    }

    bool available() const { return available_.load(std::memory_order_acquire); }
    unsigned long lastUpdateMs() const { return lastUpdateMs_; }

 private:
    std::atomic<bool> available_{false};
    unsigned long lastUpdateMs_ = 0;
};
