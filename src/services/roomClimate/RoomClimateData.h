#pragma once

#include <Arduino.h>

#include "utils/PublishedFlag.h"

// Minimal subset of the local room sensor's response (GET /getData ->
// {"temperature":26.55,"humidity":45.08}). Scalars only, same convention as
// AirQualityData — no mutex needed.
struct RoomClimateData {
    // freshness.publish() is the cross-core publish point — see
    // DataFreshnessGuard for the happens-before argument.
    PublishedFlag freshness;

    int16_t temperature_x10 = 0;  // deg C x10 (26.55 -> 266)
    uint8_t humidity = 0;         // % (rounded, clamped 0..100)
};
