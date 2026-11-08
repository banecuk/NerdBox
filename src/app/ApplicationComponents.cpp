#include "ApplicationComponents.h"

#include "ui/resources/FontRegistry.h"

// Initializer list is written in the same order as the member declarations
// in ApplicationComponents.h (which is what actually governs construction
// order) so the two stay easy to keep in sync — see the comment there.
ApplicationComponents::ApplicationComponents()
    : config(AppSettings::withEnvironment()),
      platform(data.systemState.core.isTimeSynced, config),
      services(platform.networkManager, platform.logger_, config, data.audioData),
      jobs(platform, data, services, config),
      uiController(platform.displayContext, platform.displayManager, services.systemMetrics,
                   data.systemState.screen, config),
      screenCtx{platform.logger_,
                platform.displayContext.getScreenLogQueue(),
                platform.displayManager,
                data.pcMetrics,
                uiController,
                config,
                services.systemMetrics,
                data.airQualityData,
                data.netStatus,
                data.weatherData,
                data.audioData,
                *data.cpuClockData,
                *data.processData,
                data.roomClimateData,
                *data.wifiScanData},
      taskManager(platform.logger_, uiController, config, data.systemState.screen, jobs.asVector()),
      webServer(80),
      // WebDataSources is built here once, as a temporary WebServerService
      // copies (it's a struct of references) — see WebDataSources.h (A3).
      webServerService(
          webServer,
          WebDataSources{{config, services.systemMetrics, data.pcMetrics, *jobs.pcMetricsStreamJob,
                          *jobs.cpuClockStreamJob, *jobs.processStreamJob, data.netStatus,
                          data.systemState, data.weatherData, data.roomClimateData, data.audioData,
                          *data.wifiScanData, taskManager},
                         uiController,
                         services.pcMetricsService,
                         services.audioService,
                         platform.logger_,
                         platform.logger_}),
      initStateMachine(*this, config) {
    // uiController is constructed before screenCtx (declaration order) and
    // screenCtx holds a reference back to it, so the reverse link — giving
    // uiController the address of screenCtx — can only happen here, once
    // both members exist. See the comment on ScreenCreationContext screenCtx
    // in ApplicationComponents.h.
    uiController.bindScreenContext(screenCtx);
}

// -----------------------------------------------------------------------
// IInitializationTarget implementation
// -----------------------------------------------------------------------
LoggerInterface& ApplicationComponents::logger() {
    return platform.logger_;
}

void ApplicationComponents::initializeDisplay() {
    platform.displayManager.initialize();
    Fonts::init();
}
void ApplicationComponents::postInitializeDisplay() {
    platform.displayManager.postInitialization();
}

bool ApplicationComponents::initializeUi() {
    return uiController.initialize();
}
void ApplicationComponents::requestScreen(ScreenName screen) {
    uiController.requestScreen(screen);
}

bool ApplicationComponents::initWatchdog(uint32_t timeoutMs) {
    return taskManager.initWatchdog(timeoutMs);
}
bool ApplicationComponents::addCurrentTaskToWatchdog() {
    return taskManager.addCurrentTaskToWatchdog();
}

bool ApplicationComponents::createTasks() {
    return taskManager.createTasks();
}

bool ApplicationComponents::connectNetwork() {
    return platform.networkManager.connect();
}
bool ApplicationComponents::isNetworkConnected() const {
    return platform.networkManager.isConnected();
}

bool ApplicationComponents::syncTime() {
    return platform.ntpService.syncTime();
}

void ApplicationComponents::beginWebServer() {
    webServerService.begin();
}

void ApplicationComponents::setScreenInitialized() {
    data.systemState.screen.isInitialized = true;
}
void ApplicationComponents::setSystemInitialized() {
    data.systemState.core.isInitialized = true;
}

void ApplicationComponents::delayMs(uint32_t ms) {
    delay(ms);
}
uint32_t ApplicationComponents::randomBelow(uint32_t bound) {
    return bound == 0 ? 0 : static_cast<uint32_t>(random(0, static_cast<long>(bound)));
}
uint32_t ApplicationComponents::freeHeap() const {
    return ESP.getFreeHeap();
}
