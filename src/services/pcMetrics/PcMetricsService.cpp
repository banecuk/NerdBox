#include "PcMetricsService.h"

#include <HTTPClient.h>

#include "services/pcMetrics/PcMetricsParser.h"

PcMetricsService::PcMetricsService(NetworkManager& networkManager,
                                   ApplicationMetrics& systemMetrics, LoggerInterface& logger,
                                   const AppSettings& config)
    : networkManager_(networkManager),
      systemMetrics_(systemMetrics),
      logger_(logger),
      config_(config) {
    doc_ = std::make_unique<JsonDocument>();
    initFilter();
}

void PcMetricsService::initFilter() {
    PcMetricsParser::buildFilter(filter_);
}

bool PcMetricsService::fetchData(PcMetrics& outData) {
    // Deserializes straight from the HTTP stream into doc_ — no intermediate
    // String buffer for the response body — and reuses doc_ across fetches
    // to avoid heap fragmentation.
    doc_->clear();
    HttpClient& http = networkManager_.getHttpClient();
    if (!http.downloadAndParse(LIBRE_HM_API, *doc_, filter_)) {
        if (http.getLastHttpCode() == HTTP_CODE_OK) {
            logger_.errorf("JSON parsing failed: %s", http.getLastParseError().c_str());
            snprintf(lastError_, sizeof(lastError_), "parse: %s", http.getLastParseError().c_str());
        } else {
            logger_.error("Failed to fetch data from PC metrics API");
            snprintf(lastError_, sizeof(lastError_), "http: code %d", http.getLastHttpCode());
        }
        fetchFail_++;
        return false;
    }

    const bool ok = parseData(outData);
    if (ok) {
        fetchOk_++;
        lastError_[0] = '\0';
    } else {
        fetchFail_++;
        snprintf(lastError_, sizeof(lastError_), "parse: incomplete Metrics object");
    }
    return ok;
}

bool PcMetricsService::fetchRawJson(String& outRaw) {
    HTTPClient http;
    if (!http.begin(LIBRE_HM_API)) {
        return false;
    }
    http.setConnectTimeout(1000);
    http.setTimeout(2000);
    const int code = http.GET();
    const bool ok = (code == HTTP_CODE_OK);
    if (ok) {
        outRaw = http.getString();
    }
    http.end();
    return ok;
}

bool PcMetricsService::parseData(PcMetrics& outData) {
    unsigned long startTime = millis();

    JsonObject metrics = (*doc_)["Metrics"];
    if (metrics.isNull()) {
        logger_.error("No Metrics object found in JSON");
        return false;
    }

    bool allComponentsValid = true;

    // Parse individual components with error isolation
    JsonObject cpu = metrics["Cpu"];
    if (!cpu.isNull()) {
        if (!PcMetricsParser::parseCpuData(cpu, outData, config_.pcMetricsCores, logger_)) {
            allComponentsValid = false;
        }
    }

    JsonObject cpuExtended = metrics["CpuExtended"];
    if (!cpuExtended.isNull()) {
        if (!PcMetricsParser::parseCpuExtendedData(cpuExtended, outData)) {
            allComponentsValid = false;
        }
    }

    JsonObject ram = metrics["Ram"];
    if (!ram.isNull()) {
        if (!PcMetricsParser::parseRamData(ram, outData)) {
            allComponentsValid = false;
        }
    }

    JsonObject gpu = metrics["Gpu"];
    if (!gpu.isNull()) {
        if (!PcMetricsParser::parseGpuData(gpu, outData)) {
            allComponentsValid = false;
        }
    }

    JsonObject motherboard = metrics["Motherboard"];
    if (!motherboard.isNull()) {
        if (!PcMetricsParser::parseMotherboardData(motherboard, outData)) {
            allComponentsValid = false;
        }
    }

    JsonObject disks = metrics["Disks"];
    if (!disks.isNull()) {
        if (!PcMetricsParser::parseDiskData(disks, outData)) {
            allComponentsValid = false;
        }
    } else {
        logger_.debug("No Disks in filtered JSON");
    }

    JsonObject network = metrics["Network"];
    if (!network.isNull()) {
        if (!PcMetricsParser::parseNetworkData(network, outData)) {
            allComponentsValid = false;
        }
    }

    // Update metrics — sticky on success only: a partial/failed parse leaves
    // the last known-good reading and its timestamp untouched, so its age
    // keeps growing toward the staleness timeout instead of instantly
    // flipping unavailable.
    if (allComponentsValid) {
        outData.last_update_timestamp = millis();
        outData.is_available = true;
    }

    unsigned long parseTime = millis() - startTime;
    systemMetrics_.setPcMetricsJsonParseTime(parseTime);

    return allComponentsValid;
}