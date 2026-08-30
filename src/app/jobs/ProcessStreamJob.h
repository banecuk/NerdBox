#pragma once

#include "app/jobs/SseStreamJob.h"
#include "config/AppSettings.h"
#include "core/ScreenTypes.h"
#include "core/state/SystemState.h"
#include "network/NetworkManager.h"
#include "services/processes/ProcessData.h"
#include "services/processes/ProcessStreamService.h"
#include "utils/logging/LoggerInterface.h"

// Streaming counterpart to ProcessStreamService: keeps one SseConnection
// open to NerdWinSense's opt-in top-N process list endpoint (host:port taken
// from LIBRE_HM_API, path from config.processStreamPath) via the shared
// SseStreamJob base (see docs-local/11-code-quality.md Q1) — the same
// screen-gated pattern as CpuClockStreamJob: this stream is only ever
// fetched while the PROCESSES screen is active.
class ProcessStreamJob : public SseStreamJob {
 public:
    ProcessStreamJob(ProcessData& data, SystemState::CoreState& coreState,
                     SystemState::ScreenState& screenState, NetworkManager& networkManager,
                     const AppSettings& config, LoggerInterface& logger);

 protected:
    bool screenGateOpen() const override;
    bool extraGateOpen() const override;
    void onEvent(const SseEventParser::Event& event) override;

 private:
    SystemState::CoreState& coreState_;
    SystemState::ScreenState& screenState_;
    ProcessStreamService streamService_;
};
