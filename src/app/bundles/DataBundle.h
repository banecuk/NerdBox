#pragma once

#include "core/state/SystemState.h"
#include "services/airQuality/AirQualityData.h"
#include "services/audio/AudioData.h"
#include "services/network/NetworkStatus.h"
#include "services/pcMetrics/PcMetrics.h"
#include "services/weather/WeatherData.h"

// The shared data structs written by background services/jobs and read by
// UI widgets. Grouped here only because they have no construction-order
// dependency on anything else in ApplicationComponents — every member below
// is independently default-constructible.
struct DataBundle {
    SystemState systemState;

    // Not nested in SystemState to avoid dragging the heavy PcMetrics type —
    // which includes a std::vector and a FreeRTOS semaphore — into every
    // translation unit that needs SystemState.
    PcMetrics pcMetrics;

    // Written by AirQualityService in the background task, read by
    // AirQualityWidget in the screen task. All fields are scalar so no mutex
    // is required (Xtensa word reads are atomic).
    AirQualityData airQualityData;

    // Written by NetworkStatusService (background + probe tasks), read by
    // NetworkWidget (screen task). All scalar fields — no mutex needed.
    NetworkStatus netStatus;

    // Written by WeatherService in the background task every ~2h regardless
    // of active screen, read by WeatherWidget in the screen task. Scalars
    // plus a fixed day array (no heap); only the refreshRequested flag is
    // shared cross-task and so is atomic.
    WeatherData weatherData;

    // Written by AudioService from the web server's POST /audio handler
    // (main-loop task) as the mb_NerdBox MusicBee plugin pushes now-playing
    // events, read by AudioWidget/MultiWidget (screen task). Scalars plus
    // fixed char arrays — no mutex needed, see AudioData's own comment.
    AudioData audioData;
};
