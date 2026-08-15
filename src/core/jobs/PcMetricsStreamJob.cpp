#include "PcMetricsStreamJob.h"

#include "network/UrlUtils.h"
#include "utils/LogMacros.h"

PcMetricsStreamJob::PcMetricsStreamJob(PcMetrics& metrics, SystemState::CoreState& coreState,
                                       SystemState::ScreenState& screenState,
                                       NetworkManager& networkManager, const AppSettings& config,
                                       LoggerInterface& logger)
    : metrics_(metrics),
      coreState_(coreState),
      screenState_(screenState),
      networkManager_(networkManager),
      config_(config),
      logger_(logger),
      connection_(logger, config.pcMetricsStreamMaxEventBufferBytes,
                  config.pcMetricsStreamMaxBytesPerPoll),
      streamService_(metrics, config, logger) {
    UrlUtils::parseHostPort(LIBRE_HM_API, host_, sizeof(host_), port_);
}

JobDue PcMetricsStreamJob::nextDue() const {
    const bool onMetricsScreen = screenState_.activeScreen == ScreenName::MAIN ||
                                 screenState_.activeScreen == ScreenName::GAME ||
                                 screenState_.activeScreen == ScreenName::DISKS;
    if (!config_.pcMetricsStreamEnabled || !coreState_.isInitialized || !onMetricsScreen ||
        !networkManager_.isConnected()) {
        return JobDue::never();
    }

    if (connection_.state() == SseConnection::State::Connected) {
        return JobDue::now();  // steady state: poll() every tick, never idle while connected
    }
    return JobDue::at(nextReconnectAttemptMs_);
}

void PcMetricsStreamJob::run() {
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
        reconnectCount_++;
        nextReconnectAttemptMs_ = millis() + config_.pcMetricsStreamReconnectBackoffMs;
        logger_.warning("SSE stream disconnected — will retry", true);
        return;
    }

    if (millis() - lastEventMs_ > config_.pcMetricsStreamStaleTimeoutMs) {
        // The socket can still look Connected here — WiFiClient::connected()
        // may keep reporting true on a half-open TCP connection even after
        // the peer is gone — but no event, not even NerdWinSense's
        // per-interval heartbeat, has arrived for kStaleTimeoutMs. Waiting on
        // a socket-level failure that may never come is what let this go
        // unnoticed before; force the reconnect ourselves instead.
        connection_.disconnect();
        reconnectCount_++;
        nextReconnectAttemptMs_ = millis() + config_.pcMetricsStreamReconnectBackoffMs;
        logger_.warning("SSE stream stalled (no events received) — forcing reconnect", true);
    }
}

void PcMetricsStreamJob::attemptConnect() {
    char pathWithQuery[96];
    snprintf(pathWithQuery, sizeof(pathWithQuery), "%s?intervalMs=%lu&delta=%s",
             config_.pcMetricsStreamPath,
             static_cast<unsigned long>(config_.pcMetricsStreamIntervalMs),
             config_.pcMetricsStreamDelta ? "true" : "false");

    if (connection_.connect(host_, port_, pathWithQuery, config_.pcMetricsStreamConnectTimeoutMs,
                            config_.pcMetricsStreamHeaderTimeoutMs)) {
        logger_.info("SSE stream connected", true);
        lastEventMs_ = millis();
        return;
    }

    reconnectCount_++;
    nextReconnectAttemptMs_ = millis() + config_.pcMetricsStreamReconnectBackoffMs;
    LOG_DEBUG(logger_, "SSE connect attempt failed", true);
}
