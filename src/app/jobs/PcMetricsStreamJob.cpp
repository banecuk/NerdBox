#include "PcMetricsStreamJob.h"

#include <cstdio>

#include "core/ScreenRegistry.h"

PcMetricsStreamJob::PcMetricsStreamJob(PcMetrics& metrics, SystemState::CoreState& coreState,
                                       SystemState::ScreenState& screenState,
                                       NetworkManager& networkManager, const AppSettings& config,
                                       LoggerInterface& logger, ApplicationMetrics& systemMetrics)
    : SseStreamJob(logger, networkManager, Config{"SSE stream", "metrics screen"},
                   config.pcMetricsStream, config.endpoints.pcMetricsApi),
      coreState_(coreState),
      screenState_(screenState),
      config_(config),
      streamService_(metrics, config, logger, systemMetrics) {
    char pathWithQuery[96];
    snprintf(pathWithQuery, sizeof(pathWithQuery), "%s?intervalMs=%lu&delta=%s",
             config.pcMetricsStream.streamPath,
             static_cast<unsigned long>(config.pcMetricsStream.intervalMs),
             config.pcMetricsStreamDelta ? "true" : "false");
    setPath(pathWithQuery);
}

bool PcMetricsStreamJob::screenGateOpen() const {
    return screenNeedsPcMetrics(screenState_.activeScreen);
}

bool PcMetricsStreamJob::extraGateOpen() const {
    return config_.pcMetricsStreamEnabled && coreState_.isInitialized;
}

void PcMetricsStreamJob::onEvent(const SseEventParser::Event& event) {
    streamService_.handleEvent(event);
}
