#pragma once

#include <memory>

#include "ScreenInterface.h"
#include "core/ScreenTypes.h"

// Forward declarations — avoids pulling every dependency into every TU that
// just needs ScreenFactory.
class LoggerInterface;
class DisplayManager;
class PcMetrics;
class UiController;
struct AppSettings;
class ApplicationMetrics;
class NetworkManager;
struct AirQualityData;
struct NetworkStatus;
struct WeatherData;

// Aggregates all dependencies that any screen might need.
// Pass this struct to createScreen instead of a growing parameter list;
// add new fields here as more screens are introduced.
struct ScreenCreationContext {
    LoggerInterface&      logger;
    DisplayManager*       display;
    PcMetrics&            metrics;
    UiController*         controller;
    const AppSettings&    config;
    ApplicationMetrics&   systemMetrics;
    NetworkManager&       networkManager;
    const AirQualityData& airQualityData;
    const NetworkStatus&  netStatus;
    WeatherData&          weatherData;
};

class ScreenFactory {
 public:
    static std::unique_ptr<ScreenInterface> createScreen(ScreenName name,
                                                         const ScreenCreationContext& ctx);
};
