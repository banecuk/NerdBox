#include "PcMetricsStreamJob.h"

#include <cstdio>

PcMetricsStreamJob::PcMetricsStreamJob(PcMetrics& metrics, SystemState::CoreState& coreState,
                                       SystemState::ScreenState& screenState,
                                       NetworkManager& networkManager, const AppSettings& config,
                                       LoggerInterface& logger, ApplicationMetrics& systemMetrics)
    : SseStreamJob(logger, networkManager,
                   Config{"SSE stream", "metrics screen", config.pcMetricsStreamConnectTimeoutMs,
                          config.pcMetricsStreamHeaderTimeoutMs,
                          config.pcMetricsStreamReconnectBackoffMs,
                          config.pcMetricsStreamStaleTimeoutMs},
                   config.pcMetricsStreamMaxEventBufferBytes, config.pcMetricsStreamMaxBytesPerPoll),
      coreState_(coreState),
      screenState_(screenState),
      config_(config),
      streamService_(metrics, config, logger, systemMetrics) {
    char pathWithQuery[96];
    snprintf(pathWithQuery, sizeof(pathWithQuery), "%s?intervalMs=%lu&delta=%s",
             config.pcMetricsStreamPath, static_cast<unsigned long>(config.pcMetricsStreamIntervalMs),
             config.pcMetricsStreamDelta ? "true" : "false");
    setPath(pathWithQuery);
}

bool PcMetricsStreamJob::screenGateOpen() const {
    return screenState_.activeScreen == ScreenName::MAIN ||
           screenState_.activeScreen == ScreenName::GAME ||
           screenState_.activeScreen == ScreenName::DISKS;
}

bool PcMetricsStreamJob::extraGateOpen() const {
    return config_.pcMetricsStreamEnabled && coreState_.isInitialized;
}

void PcMetricsStreamJob::onEvent(const SseEventParser::Event& event) {
    streamService_.handleEvent(event);
}
