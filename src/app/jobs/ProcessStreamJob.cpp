#include "ProcessStreamJob.h"

#include <cstdio>

ProcessStreamJob::ProcessStreamJob(ProcessData& data, SystemState::CoreState& coreState,
                                   SystemState::ScreenState& screenState,
                                   NetworkManager& networkManager, const AppSettings& config,
                                   LoggerInterface& logger)
    : SseStreamJob(logger, networkManager, Config{"Process SSE stream", "screen"},
                   config.processStream, config.endpoints.pcMetricsApi),
      coreState_(coreState),
      screenState_(screenState),
      streamService_(data, logger) {
    char pathWithQuery[96];
    snprintf(pathWithQuery, sizeof(pathWithQuery), "%s?intervalMs=%lu",
             config.processStream.streamPath,
             static_cast<unsigned long>(config.processStream.intervalMs));
    setPath(pathWithQuery);
}

bool ProcessStreamJob::screenGateOpen() const {
    return screenState_.activeScreen == ScreenName::PROCESSES;
}

bool ProcessStreamJob::extraGateOpen() const {
    return coreState_.isInitialized;
}

void ProcessStreamJob::onEvent(const SseEventParser::Event& event) {
    streamService_.handleEvent(event);
}
