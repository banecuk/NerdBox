#include "Application.h"

#include "ApplicationComponents.h"
#include "core/TaskManager.h"

constexpr const char* Application::INIT_STATE_NAMES_[];

// Updated constructor - now much simpler!
Application::Application(std::unique_ptr<ApplicationComponents> components)
    : components_(std::move(components)),
      taskManager_(components_->logger, components_->uiController,
                   components_->pcMetricsService, components_->systemState.pcMetrics,
                   components_->systemState.core, components_->systemState.screen,
                   components_->config),
      currentInitState_(InitState::INITIAL) {
    // All complex initialization is now handled by ApplicationComponents
}

bool Application::initialize() {
    components_->logger.info("Application initialization started", true);

    while (!isTerminalState()) {
        if (!handleStateTransition()) {
            handleInitializationFailure();
            return false;
        }
    }

    components_->logger.info("Application initialization completed successfully", true);
    components_->logger.debugf("Free heap post-init: %d", ESP.getFreeHeap());
    components_->uiController.requestScreen(ScreenName::MAIN);
    components_->systemState.core.isInitialized = true;
    return true;
}

void Application::run() {
    if (!components_->systemState.core.isInitialized) {
        return;
    }

    if (components_->config.getWatchdogEnableOnBoot()) {
        esp_task_wdt_reset();
    }

    components_->httpServer.processRequests();
    vTaskDelay(components_->config.getTimingMainLoopMs());
}

// Initialization Methods -----------------------------------------------------

bool Application::initializeDisplay() {
    components_->logger.info("Initializing display", true);
    components_->displayManager.initialize();
    components_->systemState.screen.isInitialized = true;
    components_->uiController.initialize();
    return true;
}

bool Application::initializeNetwork(uint8_t maxRetries) {
    components_->logger.info("Connecting to WiFi", true);
    return components_->networkManager.connect();
}

bool Application::initializeTimeService(uint8_t maxRetries) {
    components_->logger.info("Syncing time", true);

    for (uint8_t attempt = 1; attempt <= maxRetries; ++attempt) {
        if (components_->ntpService.syncTime()) {
            components_->logger.info("Time synchronized successfully", true);
            components_->systemState.core.isTimeSynced = true;
            return true;
        }

        logRetryAttempt("Time sync", attempt, maxRetries);
        delay(calculateBackoffDelay(attempt,
                                    components_->config.getTimeSyncRetryDelayBaseMs()));
    }

    components_->logger.warning("Time sync failed, using local time", true);
    return false;
}

bool Application::initializeWatchdog() {
    if (!components_->config.getWatchdogEnableOnBoot()) {
        components_->logger.info("Watchdog disabled in configuration", true);
        return true;
    }

    esp_err_t ret =
        esp_task_wdt_init(components_->config.getWatchdogTimeoutMs() / 1000, true);
    if (ret != ESP_OK) {
        components_->logger.error(
            "Failed to initialize watchdog: " + String(esp_err_to_name(ret)), true);
        return false;
    }

    ret = esp_task_wdt_add(nullptr);
    if (ret != ESP_OK) {
        components_->logger.error(
            "Failed to add main task to watchdog: " + String(esp_err_to_name(ret)), true);
        return false;
    }

    components_->logger.infof("Watchdog initialized with %dms timeout",
                              components_->config.getWatchdogTimeoutMs(), true);
    return true;
}

void Application::completeInitialization() {
    if (components_->networkManager.isConnected()) {
        components_->httpServer.begin();
        components_->logger.info("HTTP Server started", true);
    } else {
        components_->logger.warning("HTTP Server skipped: No network", true);
    }
}

// State Management -----------------------------------------------------------

bool Application::handleStateTransition() {
    switch (currentInitState_) {
        case InitState::INITIAL:
            transitionTo(InitState::DISPLAY_INIT);
            return true;

        case InitState::DISPLAY_INIT:
            return initializeDisplay() ? (transitionTo(InitState::TASKS_INIT), true)
                                       : (transitionTo(InitState::FAILED), false);

        case InitState::TASKS_INIT:
            if (!taskManager_.createTasks()) {
                components_->logger.error("Task creation failed", true);
                transitionTo(InitState::FAILED);
                return false;
            }
            transitionTo(InitState::NETWORK_INIT);
            return true;

        case InitState::NETWORK_INIT:
            if (!initializeNetwork(components_->config.getDefaultNetworkRetries())) {
                components_->logger.warning("Network init failed, continuing", true);
            }
            transitionTo(InitState::TIME_INIT);
            return true;

        case InitState::TIME_INIT:
            if (!initializeTimeService(components_->config.getDefaultTimeSyncRetries())) {
                components_->logger.warning("Time sync failed, continuing", true);
            }
            transitionTo(InitState::WATCHDOG_INIT);
            return true;

        case InitState::WATCHDOG_INIT:
            if (!initializeWatchdog()) {
                components_->logger.warning("Watchdog init failed, continuing", true);
            }
            transitionTo(InitState::FINAL_SETUP);
            return true;

        case InitState::FINAL_SETUP:
            completeInitialization();
            transitionTo(InitState::COMPLETE);
            return true;

        default:
            components_->logger.error("Unknown initialization state", true);
            transitionTo(InitState::FAILED);
            return false;
    }
}

void Application::transitionTo(InitState newState) {
    components_->logger.debug(getStateName(currentInitState_) + " -> " +
                              getStateName(newState));
    currentInitState_ = newState;
}

bool Application::isTerminalState() const {
    return currentInitState_ == InitState::COMPLETE ||
           currentInitState_ == InitState::FAILED;
}

// Logging Helpers ------------------------------------------------------------

String Application::getStateName(InitState state) const {
    return INIT_STATE_NAMES_[static_cast<int>(state)];
}

void Application::logRetryAttempt(const char* component, uint8_t attempt,
                                  uint8_t maxRetries) const {
    // components_->logger.warningf("%s init attempt %d/%d failed", component, attempt,
    // maxRetries);
}

uint16_t Application::calculateBackoffDelay(uint8_t attempt, uint16_t baseDelay) const {
    return baseDelay * (1 << (attempt - 1)) +
           (random(0, components_->config.getBackoffJitterMs()));
}

void Application::handleInitializationFailure() {
    components_->logger.critical("[FATAL] Application initialization failed in state: " +
                                 getStateName(currentInitState_));
}