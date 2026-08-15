#pragma once

#include <Arduino.h>

#include "config/AppSettings.h"
#include "config/Environment.h"
#include "core/BackgroundJob.h"
#include "core/ScreenTypes.h"
#include "core/state/SystemState.h"
#include "network/NetworkManager.h"
#include "services/pcMetrics/PcMetrics.h"
#include "services/pcMetrics/PcMetricsStreamService.h"
#include "services/pcMetrics/SseConnection.h"
#include "utils/logging/LoggerInterface.h"

// Streaming counterpart to PcMetricsJob: keeps one SseConnection open to
// NerdWinSense's SSE endpoint (host:port taken from LIBRE_HM_API, path from
// config.pcMetricsStreamPath), handling connect/reconnect-backoff/staleness
// scheduling itself. Each event's `data:` payload is handed to
// PcMetricsStreamService, which owns the JsonDocument/filter/parse/publish
// logic — the same PcMetricsParser per-section functions the polling path
// uses (SSE-PUSH-PLAN.md design constraint 5). Gated by
// config.pcMetricsStreamEnabled (default true); ApplicationComponents also
// makes PcMetricsJob::nextDue() return JobDue::never() while this is
// enabled, so only one of the two is ever due.
class PcMetricsStreamJob : public BackgroundJob {
 public:
    PcMetricsStreamJob(PcMetrics& metrics, SystemState::CoreState& coreState,
                       SystemState::ScreenState& screenState, NetworkManager& networkManager,
                       const AppSettings& config, LoggerInterface& logger);

    JobDue nextDue() const override;
    void run() override;

    SseConnection::State connectionState() const { return connection_.state(); }
    uint32_t reconnectCount() const { return reconnectCount_; }
    unsigned long lastEventAgeMs() const { return millis() - lastEventMs_; }
    uint32_t overflowCount() const { return connection_.overflowCount(); }

 private:
    void attemptConnect();
    bool screenGateOpen() const;

    PcMetrics& metrics_;
    SystemState::CoreState& coreState_;
    SystemState::ScreenState& screenState_;
    NetworkManager& networkManager_;
    const AppSettings& config_;
    LoggerInterface& logger_;

    SseConnection connection_;
    PcMetricsStreamService streamService_;

    char host_[64] = "";
    uint16_t port_ = 80;

    unsigned long nextReconnectAttemptMs_ = 0;
    unsigned long lastEventMs_ = 0;
    uint32_t reconnectCount_ = 0;
};
