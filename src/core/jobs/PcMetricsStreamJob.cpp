#include "PcMetricsStreamJob.h"

#include <cstring>

#include "utils/LogMacros.h"

namespace {

// Splits a "http://host[:port][/path]" URL into host/port — LIBRE_HM_API
// and the SSE stream live on the same NerdWinSense server, just different
// paths, so there's no separate stream-host config to maintain.
void parseHostPort(const char* url, char* outHost, size_t outHostSize, uint16_t& outPort) {
    outPort = 80;
    const char* schemeEnd = strstr(url, "://");
    const char* hostStart = schemeEnd ? schemeEnd + 3 : url;

    const char* colon = strchr(hostStart, ':');
    const char* slash = strchr(hostStart, '/');
    const char* hostEnd = colon ? colon : (slash ? slash : hostStart + strlen(hostStart));

    size_t hostLen = static_cast<size_t>(hostEnd - hostStart);
    if (hostLen >= outHostSize) {
        hostLen = outHostSize - 1;
    }
    memcpy(outHost, hostStart, hostLen);
    outHost[hostLen] = '\0';

    if (colon && (!slash || colon < slash)) {
        outPort = static_cast<uint16_t>(atoi(colon + 1));
    }
}

}  // namespace

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
      doc_(std::make_unique<JsonDocument>()) {
    PcMetricsParser::buildFilter(filter_);
    parseHostPort(LIBRE_HM_API, host_, sizeof(host_), port_);
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

    connection_.poll([this](const SseEventParser::Event& event) { handleEvent(event); });

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

void PcMetricsStreamJob::handleEvent(const SseEventParser::Event& event) {
    lastEventMs_ = millis();

    doc_->clear();
    const DeserializationError err =
        deserializeJson(*doc_, event.data, event.dataLen, DeserializationOption::Filter(filter_));
    if (err) {
        logger_.warningf("SSE event JSON parse failed: %s", err.c_str());
        return;
    }

    JsonObject metricsObj = (*doc_)["Metrics"];
    if (metricsObj.isNull()) {
        // A delta event can legitimately carry no changed sections at all —
        // nothing to commit, and not an error.
        return;
    }

    // Unlike PcMetricsService::parseData (which requires every section of a
    // full report to parse OK before publishing), delta mode means most
    // events only carry a subset of sections — so any section present is
    // enough to count as a fresh update.
    const PcMetricsParser::SectionResult sections =
        PcMetricsParser::parseAllSections(metricsObj, metrics_, config_.pcMetricsCores, logger_);

    if (sections.anySeen()) {
        metrics_.freshness.publish(millis());
    }
}
