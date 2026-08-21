#pragma once

#include <Arduino.h>

#include "config/AppSettings.h"
#include "config/Environment.h"
#include "core/BackgroundJob.h"
#include "core/ScreenTypes.h"
#include "core/state/SystemState.h"
#include "network/NetworkManager.h"
#include "services/cpuClock/CpuClockData.h"
#include "services/cpuClock/CpuClockStreamService.h"
#include "services/pcMetrics/SseConnection.h"
#include "utils/logging/LoggerInterface.h"

// Streaming counterpart to CpuClockStreamService: keeps one SseConnection
// open to NerdWinSense's opt-in per-core CPU clock endpoint (host:port taken
// from LIBRE_HM_API, path from config.cpuClockStreamPath), handling
// connect/reconnect-backoff/staleness scheduling itself — mirrors
// PcMetricsStreamJob almost exactly, with one difference in the gate: this
// stream is only ever fetched while the CPU_CLOCK screen is active, never in
// the background and never as part of the main PcMetrics stream/poll path.
class CpuClockStreamJob : public BackgroundJob {
 public:
    CpuClockStreamJob(CpuClockData& data, SystemState::ScreenState& screenState,
                      NetworkManager& networkManager, const AppSettings& config,
                      LoggerInterface& logger);

    JobDue nextDue() const override;
    void run() override;

 private:
    void attemptConnect();
    bool screenGateOpen() const;

    CpuClockData& data_;
    SystemState::ScreenState& screenState_;
    NetworkManager& networkManager_;
    const AppSettings& config_;
    LoggerInterface& logger_;

    SseConnection connection_;
    CpuClockStreamService streamService_;

    char host_[64] = "";
    uint16_t port_ = 80;

    unsigned long nextReconnectAttemptMs_ = 0;
    unsigned long lastEventMs_ = 0;
};
