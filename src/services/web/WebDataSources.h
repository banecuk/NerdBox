#pragma once

#include "config/AppSettings.h"
#include "core/IStreamHealth.h"
#include "core/ITaskStackReporter.h"
#include "core/state/SystemState.h"
#include "services/audio/AudioData.h"
#include "services/network/NetworkStatus.h"
#include "services/pcMetrics/PcMetrics.h"
#include "services/roomClimate/RoomClimateData.h"
#include "services/weather/WeatherData.h"
#include "services/wifiScan/WifiScanData.h"
#include "utils/ApplicationMetrics.h"

// Forward declarations only: WebFeeds below must stay host-constructible
// (test/MetricsFormatTest.cpp builds one under [env:native]), so this header
// can't pull in the WiFi/HTTPClient-dependent service headers.
class AudioService;
class IScreenNavigator;
class LoggerInterface;
class PcMetricsService;
class RecentLogView;

// Everything the pure web emitters (WebEmitters.h) read: shared data structs
// and narrow read-only interfaces, nothing that touches hardware. A plain
// struct of references, so adding a feed is one member here plus one line in
// ApplicationComponents.cpp instead of a parameter threaded through three
// constructors (A3).
struct WebFeeds {
    const AppSettings& config;
    const ApplicationMetrics& systemMetrics;
    const PcMetrics& pcMetrics;
    const IStreamHealth& pcMetricsStream;
    const IStreamHealth& cpuClockStream;
    const IStreamHealth& processStream;
    const NetworkStatus& netStatus;
    const SystemState& systemState;
    const WeatherData& weatherData;
    const RoomClimateData& roomClimateData;
    const AudioData& audioData;
    const WifiScanData& wifiScanData;
    const ITaskStackReporter& taskStacks;
};

// Every dependency the web server needs, built once in ApplicationComponents
// (like ScreenCreationContext) and handed to WebServerService, which passes
// it on to WebApiHandlers/WebPageHandlers. `feeds` is the read-only half the
// emitters consume; the rest are the services/sinks that only the
// firmware-side handlers use.
struct WebDataSources {
    WebFeeds feeds;
    IScreenNavigator& screenNavigator;
    PcMetricsService& pcMetricsService;
    AudioService& audioService;
    RecentLogView& recentLogView;
    LoggerInterface& logger;
};
