#pragma once

#include "app/jobs/SseStreamJob.h"
#include "config/AppSettings.h"
#include "core/ScreenTypes.h"
#include "core/state/SystemState.h"
#include "network/NetworkManager.h"
#include "services/pcMetrics/PcMetrics.h"
#include "services/pcMetrics/PcMetricsStreamService.h"
#include "utils/ApplicationMetrics.h"
#include "utils/logging/LoggerInterface.h"

// Streaming counterpart to PcMetricsJob: keeps one SseConnection open to
// NerdWinSense's SSE endpoint (host:port taken from LIBRE_HM_API, path from
// config.pcMetricsStreamPath) via the shared SseStreamJob base (see
// docs-local/11-code-quality.md Q1). Each event's `data:` payload is handed
// to PcMetricsStreamService, which owns the JsonDocument/filter/parse/publish
// logic — the same PcMetricsParser per-section functions the polling path
// uses (SSE-PUSH-PLAN.md design constraint 5). Gated by
// config.pcMetricsStreamEnabled (default true, see extraGateOpen());
// ApplicationComponents also makes PcMetricsJob::nextDue() return
// JobDue::never() while this is enabled, so only one of the two is ever due.
class PcMetricsStreamJob : public SseStreamJob {
 public:
    PcMetricsStreamJob(PcMetrics& metrics, SystemState::CoreState& coreState,
                       SystemState::ScreenState& screenState, NetworkManager& networkManager,
                       const AppSettings& config, LoggerInterface& logger,
                       ApplicationMetrics& systemMetrics);

 protected:
    bool screenGateOpen() const override;
    bool extraGateOpen() const override;
    void onEvent(const SseEventParser::Event& event) override;

 private:
    SystemState::CoreState& coreState_;
    SystemState::ScreenState& screenState_;
    const AppSettings& config_;

    PcMetricsStreamService streamService_;
};
