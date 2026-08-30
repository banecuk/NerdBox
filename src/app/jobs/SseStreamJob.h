#pragma once

#include <Arduino.h>

#include <cstddef>
#include <cstdint>

#include "core/BackgroundJob.h"
#include "network/NetworkManager.h"
#include "services/pcMetrics/SseConnection.h"
#include "services/pcMetrics/SseEventParser.h"
#include "utils/logging/LoggerInterface.h"

// Shared connect/reconnect-backoff/staleness-watchdog machinery for a
// screen-gated SSE stream job. PcMetricsStreamJob, CpuClockStreamJob and
// ProcessStreamJob were three ~100-line copies of this same state machine
// (same guard order, same "off-screen: tear down once then idle", same
// stale-timeout half-open-TCP guard) — see docs-local/11-code-quality.md Q1.
//
// A derived class supplies: a Config (log wording + per-stream timeouts),
// the request path via setPath() (host:port always comes from LIBRE_HM_API,
// resolved here), its own screenGateOpen() rule, and an onEvent() that
// forwards to its *StreamService. nextDue()/run() are `final` — the
// scheduling shape is exactly the point of sharing this base.
class SseStreamJob : public BackgroundJob {
 public:
    struct Config {
        const char* logPrefix;       // e.g. "SSE stream", "CPU clock SSE stream" — static storage
        const char* offScreenScope;  // e.g. "metrics screen", "screen" — static storage
        uint16_t connectTimeoutMs;
        uint16_t headerTimeoutMs;
        uint32_t reconnectBackoffMs;
        uint32_t staleTimeoutMs;
    };

    SseStreamJob(LoggerInterface& logger, NetworkManager& networkManager, Config config,
                 size_t eventBufferBytes, uint16_t maxBytesPerPoll);

    JobDue nextDue() const final;
    void run() final;

    SseConnection::State connectionState() const { return connection_.state(); }
    uint32_t reconnectCount() const { return reconnectCount_; }
    unsigned long lastEventAgeMs() const { return millis() - lastEventMs_; }
    uint32_t overflowCount() const { return connection_.overflowCount(); }

 protected:
    // Whether the screen this stream feeds is currently active. Off-screen,
    // an open connection is torn down once (run() still fires once more for
    // that) and the job then goes idle — reconnecting just to sit unread
    // would be pointless (see B3).
    virtual bool screenGateOpen() const = 0;

    // Extra runtime precondition beyond WiFi connectivity + the screen gate.
    // Only PcMetricsStreamJob has one (pcMetricsStreamEnabled &&
    // coreState_.isInitialized); the other two streams use the default.
    virtual bool extraGateOpen() const { return true; }

    virtual void onEvent(const SseEventParser::Event& event) = 0;

    // Derived ctor calls this once, from its constructor body, with the
    // request path already including the query string (built from
    // AppSettings values that don't change at runtime).
    void setPath(const char* pathWithQuery);

 private:
    void attemptConnect();
    void scheduleReconnect();

    LoggerInterface& logger_;
    NetworkManager& networkManager_;
    Config config_;

    SseConnection connection_;

    char host_[64] = "";
    uint16_t port_ = 80;
    char path_[96] = "";

    unsigned long nextReconnectAttemptMs_ = 0;
    unsigned long lastEventMs_ = 0;
    uint32_t reconnectCount_ = 0;
};
