#pragma once

#include <cstdint>

#include "utils/PublishedFlag.h"

// Plain data struct for the opt-in top-N process list SSE stream
// (GET /api/v1/stream/processes), mirroring CpuClockData's shape. Fixed
// arrays only (~880 B) — written on the background task, read on the screen
// task, so no mutex: same convention as PcMetrics' scalar fields and
// AudioData's strings (a torn snprintf read is cosmetic and self-heals on
// the next tick).
struct ProcessEntry {
    char name[20] = "";        // truncated; no String, no heap
    int32_t pid = 0;
    float cpuPercent = -1.0f;  // -1 == JSON null (first tick on a fresh connection)
    float ramMB = 0.0f;
    float diskKBPerSec = 0.0f;
};

struct ProcessData {
    static constexpr uint8_t kEntriesPerList = 8;  // endpoint's documented top-N

    ProcessEntry byCpu[kEntriesPerList];
    ProcessEntry byRam[kEntriesPerList];
    ProcessEntry byDisk[kEntriesPerList];
    uint8_t byCpuCount = 0;
    uint8_t byRamCount = 0;
    uint8_t byDiskCount = 0;
    PublishedFlag freshness;
};
