#include "InternetVerdict.h"

NetworkStatus::Internet classifyInternet(const ProbeResult* results, size_t count) {
    size_t probedCount = 0;
    size_t failures = 0;
    for (size_t i = 0; i < count; ++i) {
        if (!results[i].probed) continue;
        ++probedCount;
        if (!results[i].ok) ++failures;
    }

    if (probedCount == 0) return NetworkStatus::Internet::UNKNOWN;
    if (failures == probedCount) return NetworkStatus::Internet::DOWN;
    if (failures <= 1) return NetworkStatus::Internet::OK;
    if (failures == 2) return NetworkStatus::Internet::WARNING;
    return NetworkStatus::Internet::DEGRADED;
}
