#pragma once

#include <Arduino.h>

#include "config/AppSettings.h"
#include "config/Environment.h"
#include "core/BackgroundJob.h"
#include "core/ScreenTypes.h"
#include "core/state/SystemState.h"
#include "network/NetworkManager.h"
#include "services/pcMetrics/SseConnection.h"
#include "services/processes/ProcessData.h"
#include "services/processes/ProcessStreamService.h"
#include "utils/logging/LoggerInterface.h"

// Streaming counterpart to ProcessStreamService: keeps one SseConnection
// open to NerdWinSense's opt-in top-N process list endpoint (host:port taken
// from LIBRE_HM_API, path from config.processStreamPath), handling
// connect/reconnect-backoff/staleness scheduling itself — mirrors
// CpuClockStreamJob almost exactly, with the same screen-gated fetch: this
// stream is only ever fetched while the PROCESSES screen is active.
class ProcessStreamJob : public BackgroundJob {
 public:
    ProcessStreamJob(ProcessData& data, SystemState::ScreenState& screenState,
                     NetworkManager& networkManager, const AppSettings& config,
                     LoggerInterface& logger);

    JobDue nextDue() const override;
    void run() override;

 private:
    void attemptConnect();
    bool screenGateOpen() const;

    ProcessData& data_;
    SystemState::ScreenState& screenState_;
    NetworkManager& networkManager_;
    const AppSettings& config_;
    LoggerInterface& logger_;

    SseConnection connection_;
    ProcessStreamService streamService_;

    char host_[64] = "";
    uint16_t port_ = 80;

    unsigned long nextReconnectAttemptMs_ = 0;
    unsigned long lastEventMs_ = 0;
};
