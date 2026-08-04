#include "PcMetricsStreamJob.h"

#include <cstring>

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

unsigned long PcMetricsStreamJob::nextDueMs() const {
    const bool onMetricsScreen = screenState_.activeScreen == ScreenName::MAIN ||
                                 screenState_.activeScreen == ScreenName::GAME ||
                                 screenState_.activeScreen == ScreenName::DISKS;
    if (!config_.pcMetricsStreamEnabled || !coreState_.isInitialized || !onMetricsScreen ||
        !networkManager_.isConnected()) {
        return ULONG_MAX;
    }

    if (connection_.state() == SseConnection::State::Connected) {
        return 0;  // steady state: poll() every tick, never idle while connected
    }
    return nextReconnectAttemptMs_;
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
    }
}

void PcMetricsStreamJob::attemptConnect() {
    char pathWithQuery[96];
    snprintf(pathWithQuery, sizeof(pathWithQuery), "%s?intervalMs=%lu&delta=%s",
             config_.pcMetricsStreamPath, static_cast<unsigned long>(config_.pcMetricsStreamIntervalMs),
             config_.pcMetricsStreamDelta ? "true" : "false");

    if (connection_.connect(host_, port_, pathWithQuery, config_.pcMetricsStreamConnectTimeoutMs,
                            config_.pcMetricsStreamHeaderTimeoutMs)) {
        logger_.info("SSE stream connected", true);
        lastEventMs_ = millis();
        return;
    }

    reconnectCount_++;
    nextReconnectAttemptMs_ = millis() + config_.pcMetricsStreamReconnectBackoffMs;
    logger_.debug("SSE connect attempt failed", true);
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
    bool anyParsed = false;

    JsonObject cpu = metricsObj["Cpu"];
    if (!cpu.isNull()) {
        PcMetricsParser::parseCpuData(cpu, metrics_, config_.pcMetricsCores, logger_);
        anyParsed = true;
    }

    JsonObject cpuExtended = metricsObj["CpuExtended"];
    if (!cpuExtended.isNull()) {
        PcMetricsParser::parseCpuExtendedData(cpuExtended, metrics_);
        anyParsed = true;
    }

    JsonObject ram = metricsObj["Ram"];
    if (!ram.isNull()) {
        PcMetricsParser::parseRamData(ram, metrics_);
        anyParsed = true;
    }

    JsonObject gpu = metricsObj["Gpu"];
    if (!gpu.isNull()) {
        PcMetricsParser::parseGpuData(gpu, metrics_);
        anyParsed = true;
    }

    JsonObject motherboard = metricsObj["Motherboard"];
    if (!motherboard.isNull()) {
        PcMetricsParser::parseMotherboardData(motherboard, metrics_);
        anyParsed = true;
    }

    JsonObject disks = metricsObj["Disks"];
    if (!disks.isNull()) {
        PcMetricsParser::parseDiskData(disks, metrics_);
        anyParsed = true;
    }

    JsonObject network = metricsObj["Network"];
    if (!network.isNull()) {
        PcMetricsParser::parseNetworkData(network, metrics_);
        anyParsed = true;
    }

    if (anyParsed) {
        metrics_.last_update_timestamp = millis();
        metrics_.is_available = true;
    }
}
