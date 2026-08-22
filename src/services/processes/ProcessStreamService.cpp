#include "ProcessStreamService.h"

#include <Arduino.h>

#include <algorithm>
#include <cstdio>

ProcessStreamService::ProcessStreamService(ProcessData& data, LoggerInterface& logger)
    : data_(data), logger_(logger), doc_(std::make_unique<JsonDocument>()) {
    filter_["TopByCpu"][0]["Name"] = true;
    filter_["TopByCpu"][0]["Pid"] = true;
    filter_["TopByCpu"][0]["CpuPercent"] = true;
    filter_["TopByCpu"][0]["RamMB"] = true;
    filter_["TopByCpu"][0]["DiskKBPerSec"] = true;
    filter_["TopByRam"] = filter_["TopByCpu"];
    filter_["TopByDisk"] = filter_["TopByCpu"];
}

uint8_t ProcessStreamService::parseList(JsonArrayConst arr,
                                        ProcessEntry (&out)[ProcessData::kEntriesPerList]) {
    const size_t count = std::min<size_t>(arr.size(), ProcessData::kEntriesPerList);
    size_t i = 0;
    for (JsonObjectConst v : arr) {
        if (i >= count)
            break;
        ProcessEntry& entry = out[i];
        snprintf(entry.name, sizeof(entry.name), "%s", v["Name"] | "");
        entry.pid = v["Pid"] | 0;
        entry.cpuPercent = v["CpuPercent"].isNull() ? -1.0f : v["CpuPercent"].as<float>();
        entry.ramMB = v["RamMB"] | 0.0f;
        entry.diskKBPerSec = v["DiskKBPerSec"] | 0.0f;
        ++i;
    }
    return static_cast<uint8_t>(count);
}

void ProcessStreamService::handleEvent(const SseEventParser::Event& event) {
    doc_->clear();
    const DeserializationError err =
        deserializeJson(*doc_, event.data, event.dataLen, DeserializationOption::Filter(filter_));
    if (err) {
        logger_.warningf("Process SSE event JSON parse failed: %s", err.c_str());
        return;
    }

    bool sawAny = false;

    JsonArrayConst byCpu = (*doc_)["TopByCpu"];
    if (!byCpu.isNull()) {
        data_.byCpuCount = parseList(byCpu, data_.byCpu);
        sawAny = true;
    }

    JsonArrayConst byRam = (*doc_)["TopByRam"];
    if (!byRam.isNull()) {
        data_.byRamCount = parseList(byRam, data_.byRam);
        sawAny = true;
    }

    JsonArrayConst byDisk = (*doc_)["TopByDisk"];
    if (!byDisk.isNull()) {
        data_.byDiskCount = parseList(byDisk, data_.byDisk);
        sawAny = true;
    }

    if (sawAny) {
        data_.freshness.publish(millis());
    }
}
