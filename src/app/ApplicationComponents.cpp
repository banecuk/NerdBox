#include "ApplicationComponents.h"

#include "ui/resources/FontRegistry.h"

// Initializer list is written in the same order as the member declarations
// in ApplicationComponents.h (which is what actually governs construction
// order) so the two stay easy to keep in sync — see the comment there.
ApplicationComponents::ApplicationComponents()
    : platform(data.systemState.core.isTimeSynced, config),
      services(platform.networkManager, platform.logger_, config, data.audioData),
      jobs(platform, data, services, config),
      uiController(platform.displayContext, platform.displayManager, services.systemMetrics,
                   data.pcMetrics, data.systemState.screen, config, platform.networkManager,
                   data.airQualityData, data.netStatus, data.weatherData, data.audioData),
      taskManager(platform.logger_, uiController, config, data.systemState.screen, jobs.asVector()),
      webServer(80),
      webServerService(webServer, uiController, services.systemMetrics, data.pcMetrics,
                       services.pcMetricsService, jobs.pcMetricsStreamJob, data.netStatus,
                       data.systemState, data.weatherData, config, taskManager, platform.logger_,
                       platform.logger_, data.audioData, services.audioService),
      initStateMachine(*this) {}

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
void ApplicationComponents::setTimeSynced() {
    data.systemState.core.isTimeSynced = true;
}
void ApplicationComponents::setSystemInitialized() {
    data.systemState.core.isInitialized = true;
}

uint8_t ApplicationComponents::initTimeSyncRetries() const {
    return config.initTimeSyncRetries;
}
uint32_t ApplicationComponents::initTimeSyncBaseDelayMs() const {
    return config.initTimeSyncBaseDelayMs;
}
uint16_t ApplicationComponents::initBackoffJitterMs() const {
    return config.initBackoffJitterMs;
}
bool ApplicationComponents::watchdogEnabledOnBoot() const {
    return config.watchdogEnableOnBoot;
}
unsigned long ApplicationComponents::watchdogTimeoutMs() const {
    return config.watchdogTimeoutMs;
}
