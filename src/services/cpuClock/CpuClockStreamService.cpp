#include "CpuClockStreamService.h"

#include <Arduino.h>

#include <algorithm>

#include "config/Limits.h"

CpuClockStreamService::CpuClockStreamService(CpuClockData& data, LoggerInterface& logger)
    : data_(data), logger_(logger), doc_(std::make_unique<JsonDocument>()) {
    filter_["CoreClocksMHz"] = true;
    filter_["BusSpeedMHz"] = true;
}

void CpuClockStreamService::handleEvent(const SseEventParser::Event& event) {
    doc_->clear();
    const DeserializationError err =
        deserializeJson(*doc_, event.data, event.dataLen, DeserializationOption::Filter(filter_));
    if (err) {
        logger_.warningf("CPU clock SSE event JSON parse failed: %s", err.c_str());
        return;
    }

    bool sawAny = false;

    JsonArrayConst coreClocks = (*doc_)["CoreClocksMHz"];
    if (!coreClocks.isNull()) {
        const size_t count = std::min<size_t>(coreClocks.size(), AppConfig::Limits::kMaxThreads);
        size_t i = 0;
        for (JsonVariantConst v : coreClocks) {
            if (i >= count)
                break;
            data_.coreClockMHz[i++] = v.as<float>();
        }
        data_.coreCount = static_cast<uint8_t>(count);
        sawAny = true;
    }

    JsonVariantConst busSpeed = (*doc_)["BusSpeedMHz"];
    if (!busSpeed.isNull()) {
        data_.busSpeedMHz = busSpeed.as<float>();
        sawAny = true;
    }

    if (sawAny) {
        data_.freshness.publish(millis());
    }
}
