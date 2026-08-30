#pragma once

#include <WebServer.h>

#include "config/AppSettings.h"
#include "core/ITaskStackReporter.h"
#include "core/IStreamHealth.h"
#include "core/state/SystemState.h"
#include "services/audio/AudioData.h"
#include "services/network/NetworkStatus.h"
#include "services/pcMetrics/PcMetrics.h"
#include "services/pcMetrics/PcMetricsService.h"
#include "services/roomClimate/RoomClimateData.h"
#include "services/weather/WeatherData.h"
#include "utils/ApplicationMetrics.h"

// The /api/* JSON handlers, split out of WebServerService so that class isn't
// also responsible for hand-rolled JSON assembly. Every response is built as
// an ArduinoJson JsonDocument and streamed via ChunkedPrint — no snprintf
// argument lists to keep in sync positionally, same zero-large-buffer memory
// profile as before.
class WebApiHandlers {
 public:
    WebApiHandlers(WebServer& server, ApplicationMetrics& systemMetrics, PcMetrics& pcMetrics,
                   PcMetricsService& pcMetricsService, IStreamHealth& pcMetricsStreamJob,
                   IStreamHealth& cpuClockStreamJob, IStreamHealth& processStreamJob,
                   const NetworkStatus& netStatus, const SystemState& systemState,
                   const WeatherData& weatherData, const AppSettings& config,
                   const ITaskStackReporter& taskStackReporter, const AudioData& audioData,
                   const RoomClimateData& roomClimateData);

    void handleApiStatus();
    void handleApiRaw();
    void handleApiPc();
    void handleMetrics();

 private:
    static const char* internetStatusToString(NetworkStatus::Internet status);
    static const char* screenNameToString(ScreenName screen);
    static const char* playStateToString(AudioData::PlayState state);

    WebServer& server_;
    ApplicationMetrics& systemMetrics_;
    PcMetrics& pcMetrics_;
    PcMetricsService& pcMetricsService_;
    IStreamHealth& pcMetricsStreamJob_;
    IStreamHealth& cpuClockStreamJob_;
    IStreamHealth& processStreamJob_;
    const NetworkStatus& netStatus_;
    const SystemState& systemState_;
    const WeatherData& weatherData_;
    const AppSettings& config_;
    const ITaskStackReporter& taskStackReporter_;
    const AudioData& audioData_;
    const RoomClimateData& roomClimateData_;
};
