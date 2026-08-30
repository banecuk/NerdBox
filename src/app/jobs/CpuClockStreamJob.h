#pragma once

#include "app/jobs/SseStreamJob.h"
#include "config/AppSettings.h"
#include "core/ScreenTypes.h"
#include "core/state/SystemState.h"
#include "network/NetworkManager.h"
#include "services/cpuClock/CpuClockData.h"
#include "services/cpuClock/CpuClockStreamService.h"
#include "utils/logging/LoggerInterface.h"

// Streaming counterpart to CpuClockStreamService: keeps one SseConnection
// open to NerdWinSense's opt-in per-core CPU clock endpoint (host:port taken
// from LIBRE_HM_API, path from config.cpuClockStreamPath) via the shared
// SseStreamJob base (see docs-local/11-code-quality.md Q1). This stream is
// only ever fetched while the CPU_CLOCK screen is active, never in the
// background and never as part of the main PcMetrics stream/poll path.
class CpuClockStreamJob : public SseStreamJob {
 public:
    CpuClockStreamJob(CpuClockData& data, SystemState::CoreState& coreState,
                      SystemState::ScreenState& screenState, NetworkManager& networkManager,
                      const AppSettings& config, LoggerInterface& logger);

 protected:
    bool screenGateOpen() const override;
    bool extraGateOpen() const override;
    void onEvent(const SseEventParser::Event& event) override;

 private:
    SystemState::CoreState& coreState_;
    SystemState::ScreenState& screenState_;
    CpuClockStreamService streamService_;
};
