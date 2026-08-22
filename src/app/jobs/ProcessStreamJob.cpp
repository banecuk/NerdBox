#include "ProcessStreamJob.h"

#include "network/UrlUtils.h"
#include "utils/logging/LogMacros.h"

ProcessStreamJob::ProcessStreamJob(ProcessData& data, SystemState::ScreenState& screenState,
                                   NetworkManager& networkManager, const AppSettings& config,
                                   LoggerInterface& logger)
    : data_(data),
      screenState_(screenState),
      networkManager_(networkManager),
      config_(config),
      logger_(logger),
      connection_(logger, config.processStreamMaxEventBufferBytes,
                  config.processStreamMaxBytesPerPoll),
      streamService_(data, logger) {
    UrlUtils::parseHostPort(LIBRE_HM_API, host_, sizeof(host_), port_);
}

bool ProcessStreamJob::screenGateOpen() const {
    return screenState_.activeScreen == ScreenName::PROCESSES;
}

JobDue ProcessStreamJob::nextDue() const {
    if (!networkManager_.isConnected()) {
        return JobDue::never();
    }

    if (!screenGateOpen()) {
        // Off the processes screen: if a connection is still open, run() once
        // more to tear it down, then go idle. Otherwise there's nothing to
        // do — don't reconnect just to sit there unread.
        return connection_.state() == SseConnection::State::Connected ? JobDue::now()
                                                                       : JobDue::never();
    }

    if (connection_.state() == SseConnection::State::Connected) {
        return JobDue::now();  // steady state: poll() every tick, never idle while connected
    }
    return JobDue::at(nextReconnectAttemptMs_);
}

void ProcessStreamJob::run() {
    if (!screenGateOpen()) {
        if (connection_.state() == SseConnection::State::Connected) {
            connection_.disconnect();
            logger_.info("Process SSE stream disconnected (left screen)", true);
        }
        return;
    }

    if (connection_.state() != SseConnection::State::Connected) {
        attemptConnect();
        return;
    }

    connection_.poll([this](const SseEventParser::Event& event) {
        lastEventMs_ = millis();
        streamService_.handleEvent(event);
    });

    if (connection_.state() != SseConnection::State::Connected) {
        // poll() detected the server dropped the connection this tick.
        nextReconnectAttemptMs_ = millis() + config_.processStreamReconnectBackoffMs;
        logger_.warning("Process SSE stream disconnected — will retry", true);
        return;
    }

    if (millis() - lastEventMs_ > config_.processStreamStaleTimeoutMs) {
        // Same half-open-TCP guard as PcMetricsStreamJob: the socket can look
        // Connected here even though no event has arrived for the stale
        // timeout, so force the reconnect ourselves rather than waiting on a
        // socket-level failure that may never come.
        connection_.disconnect();
        nextReconnectAttemptMs_ = millis() + config_.processStreamReconnectBackoffMs;
        logger_.warning("Process SSE stream stalled (no events received) — forcing reconnect",
                        true);
    }
}

void ProcessStreamJob::attemptConnect() {
    char pathWithQuery[96];
    snprintf(pathWithQuery, sizeof(pathWithQuery), "%s?intervalMs=%lu", config_.processStreamPath,
             static_cast<unsigned long>(config_.processStreamIntervalMs));

    if (connection_.connect(host_, port_, pathWithQuery, config_.processStreamConnectTimeoutMs,
                            config_.processStreamHeaderTimeoutMs)) {
        logger_.info("Process SSE stream connected", true);
        lastEventMs_ = millis();
        return;
    }

    nextReconnectAttemptMs_ = millis() + config_.processStreamReconnectBackoffMs;
    LOG_DEBUG(logger_, "Process SSE connect attempt failed", true);
}
