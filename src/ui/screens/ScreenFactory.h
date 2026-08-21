#pragma once

#include <memory>

#include "core/ScreenTypes.h"
#include "ui/screens/base/ScreenInterface.h"

// Forward declarations — avoids pulling every dependency into every TU that
// just needs ScreenFactory.
class LoggerInterface;
class ScreenLogQueue;
class DisplayManager;
class PcMetrics;
class UiController;
struct AppSettings;
class ApplicationMetrics;
class NetworkManager;
struct AirQualityData;
struct NetworkStatus;
struct WeatherData;
struct AudioData;
struct CpuClockData;

// Aggregates all dependencies that any screen might need.
// Pass this struct to createScreen instead of a growing parameter list;
// add new fields here as more screens are introduced.
struct ScreenCreationContext {
    LoggerInterface& logger;
    ScreenLogQueue& screenLogQueue;
    DisplayManager* display;
    PcMetrics& metrics;
    UiController* controller;
    const AppSettings& config;
    ApplicationMetrics& systemMetrics;
    NetworkManager& networkManager;
    const AirQualityData& airQualityData;
    const NetworkStatus& netStatus;
    WeatherData& weatherData;
    const AudioData& audioData;
    CpuClockData& cpuClockData;
};

class ScreenFactory {
 public:
    static std::unique_ptr<ScreenInterface> createScreen(ScreenName name,
                                                         const ScreenCreationContext& ctx);
};
