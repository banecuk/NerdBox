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
        lastUpdateMs_.store(nowMs, std::memory_order_release);
        available_.store(true, std::memory_order_release);
    }

    bool available() const { return available_.load(std::memory_order_acquire); }

    // Atomic (not a plain field) because every publish() after the first
    // rewrites it while readers on the other core are running — the flag's
    // release/acquire pair only orders the *first* publish (B58).
    unsigned long lastUpdateMs() const { return lastUpdateMs_.load(std::memory_order_acquire); }

    // Milliseconds since the last publish(), or 0 if never published — the
    // "age" every /api/status feed object and a few widgets report.
    // Wrap-safe: unsigned subtraction across a millis() rollover still
    // yields the true elapsed time.
    unsigned long ageMs(unsigned long nowMs) const {
        return available() ? elapsedMs(nowMs, lastUpdateMs()) : 0;
    }

    // now - then, clamped to 0 when `then` is (slightly) *after* `now`.
    // Callers usually read millis() first and the timestamp second; a
    // publish on the other core in between makes the timestamp newer than
    // `now`, and the raw unsigned difference wrapped to ~4.29e9 — a
    // one-frame "stale"/"No data" flicker, or a nonsense age in
    // /api/status (B58). Treating any difference >= 2^31 as negative is
    // still wrap-safe for real ages (< 24.8 days).
    static unsigned long elapsedMs(unsigned long now, unsigned long then) {
        const long diff = static_cast<long>(now - then);
        return diff < 0 ? 0 : static_cast<unsigned long>(diff);
    }

 private:
    std::atomic<bool> available_{false};
    std::atomic<unsigned long> lastUpdateMs_{0};
};
