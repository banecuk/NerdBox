#pragma once

#include <memory>

#include "ScreenInterface.h"
#include "ui/screens/ScreenTypes.h"

// Forward declarations — avoids pulling every dependency into every TU that
// just needs ScreenFactory.
class LoggerInterface;
class DisplayManager;
class PcMetrics;
class UiController;
class AppConfigInterface;
class ApplicationMetrics;
class NetworkManager;
struct AirQualityData;

// Aggregates all dependencies that any screen might need.
// Pass this struct to createScreen instead of a growing parameter list;
// add new fields here as more screens are introduced.
struct ScreenCreationContext {
    LoggerInterface&      logger;
    DisplayManager*       display;
    PcMetrics&            metrics;
    UiController*         controller;
    AppConfigInterface&   config;
    ApplicationMetrics&   systemMetrics;
    NetworkManager&       networkManager;
    const AirQualityData& airQualityData;
};

class ScreenFactory {
 public:
    static std::unique_ptr<ScreenInterface> createScreen(ScreenName name,
                                                         const ScreenCreationContext& ctx);
};
