#include "PcMetricsStreamService.h"

#include <Arduino.h>

#include "services/pcMetrics/PcMetricsParser.h"

PcMetricsStreamService::PcMetricsStreamService(PcMetrics& metrics, const AppSettings& config,
                                               LoggerInterface& logger,
                                               ApplicationMetrics& systemMetrics)
    : metrics_(metrics),
      config_(config),
      logger_(logger),
      systemMetrics_(systemMetrics),
      doc_(std::make_unique<JsonDocument>()) {
    PcMetricsParser::buildFilter(filter_);
}

void PcMetricsStreamService::handleEvent(const SseEventParser::Event& event) {
    const uint32_t startUs = micros();

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

    systemMetrics_.setPcMetricsStreamParseTimeUs(micros() - startUs);
}
