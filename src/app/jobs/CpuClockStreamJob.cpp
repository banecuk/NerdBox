#include "CpuClockStreamJob.h"

#include <cstdio>

CpuClockStreamJob::CpuClockStreamJob(CpuClockData& data, SystemState::CoreState& coreState,
                                     SystemState::ScreenState& screenState,
                                     NetworkManager& networkManager, const AppSettings& config,
                                     LoggerInterface& logger)
    : SseStreamJob(logger, networkManager,
                   Config{"CPU clock SSE stream", "screen", config.cpuClockStreamConnectTimeoutMs,
                          config.cpuClockStreamHeaderTimeoutMs,
                          config.cpuClockStreamReconnectBackoffMs,
                          config.cpuClockStreamStaleTimeoutMs},
                   config.cpuClockStreamMaxEventBufferBytes, config.cpuClockStreamMaxBytesPerPoll),
      coreState_(coreState),
      screenState_(screenState),
      streamService_(data, logger) {
    char pathWithQuery[96];
    snprintf(pathWithQuery, sizeof(pathWithQuery), "%s?intervalMs=%lu", config.cpuClockStreamPath,
             static_cast<unsigned long>(config.cpuClockStreamIntervalMs));
    setPath(pathWithQuery);
}

bool CpuClockStreamJob::screenGateOpen() const {
    return screenState_.activeScreen == ScreenName::CPU_CLOCK;
}

bool CpuClockStreamJob::extraGateOpen() const {
    return coreState_.isInitialized;
}

void CpuClockStreamJob::onEvent(const SseEventParser::Event& event) {
    streamService_.handleEvent(event);
}
