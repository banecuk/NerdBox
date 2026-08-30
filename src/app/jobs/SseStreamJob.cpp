#include "SseStreamJob.h"

#include <cstring>

#include "config/Environment.h"
#include "network/UrlUtils.h"
#include "utils/logging/LogMacros.h"

SseStreamJob::SseStreamJob(LoggerInterface& logger, NetworkManager& networkManager, Config config,
                           size_t eventBufferBytes, uint16_t maxBytesPerPoll)
    : logger_(logger),
      networkManager_(networkManager),
      config_(config),
      connection_(logger, eventBufferBytes, maxBytesPerPoll) {
    UrlUtils::parseHostPort(LIBRE_HM_API, host_, sizeof(host_), port_);
}

void SseStreamJob::setPath(const char* pathWithQuery) {
    strncpy(path_, pathWithQuery, sizeof(path_) - 1);
    path_[sizeof(path_) - 1] = '\0';
}

JobDue SseStreamJob::nextDue() const {
    if (!networkManager_.isConnected() || !extraGateOpen()) {
        return JobDue::never();
    }

    if (!screenGateOpen()) {
        // Off-screen: if a connection is still open, run() once more to tear
        // it down, then go idle. Otherwise there's nothing to do.
        return connection_.state() == SseConnection::State::Connected ? JobDue::now()
                                                                       : JobDue::never();
    }

    if (connection_.state() == SseConnection::State::Connected) {
        return JobDue::now();  // steady state: poll() every tick, never idle while connected
    }
    return JobDue::at(nextReconnectAttemptMs_);
}

void SseStreamJob::run() {
    if (!screenGateOpen()) {
        if (connection_.state() == SseConnection::State::Connected) {
            connection_.disconnect();
            char msg[80];
            snprintf(msg, sizeof(msg), "%s disconnected (left %s)", config_.logPrefix,
                     config_.offScreenScope);
            logger_.info(msg, true);
        }
        return;
    }

    if (connection_.state() != SseConnection::State::Connected) {
        attemptConnect();
        return;
    }

    connection_.poll([this](const SseEventParser::Event& event) {
        lastEventMs_ = millis();
        onEvent(event);
    });

    if (connection_.state() != SseConnection::State::Connected) {
        // poll() detected the server dropped the connection this tick.
        scheduleReconnect();
        char msg[80];
        snprintf(msg, sizeof(msg), "%s disconnected — will retry", config_.logPrefix);
        logger_.warning(msg, true);
        return;
    }

    if (millis() - lastEventMs_ > config_.staleTimeoutMs) {
        // The socket can still look Connected here — WiFiClient::connected()
        // may keep reporting true on a half-open TCP connection even after
        // the peer is gone — but no event has arrived for staleTimeoutMs.
        // Waiting on a socket-level failure that may never come is what let
        // this go unnoticed before; force the reconnect ourselves instead.
        connection_.disconnect();
        scheduleReconnect();
        char msg[96];
        snprintf(msg, sizeof(msg), "%s stalled (no events received) — forcing reconnect",
                 config_.logPrefix);
        logger_.warning(msg, true);
    }
}

void SseStreamJob::attemptConnect() {
    if (connection_.connect(host_, port_, path_, config_.connectTimeoutMs,
                            config_.headerTimeoutMs)) {
        char msg[64];
        snprintf(msg, sizeof(msg), "%s connected", config_.logPrefix);
        logger_.info(msg, true);
        lastEventMs_ = millis();
        return;
    }

    scheduleReconnect();
    char msg[64];
    snprintf(msg, sizeof(msg), "%s connect attempt failed", config_.logPrefix);
    LOG_DEBUG(logger_, msg, true);
}

void SseStreamJob::scheduleReconnect() {
    reconnectCount_++;
    nextReconnectAttemptMs_ = millis() + config_.reconnectBackoffMs;
}
