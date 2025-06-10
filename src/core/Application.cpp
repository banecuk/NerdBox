#include "Application.h"

#include "core/TaskManager.h"
#include "network/HttpClient.h"
#include "network/NetworkManager.h"
#include "services/HttpServer.h"
#include "services/NtpService.h"
#include "services/PcMetricsService.h"
#include "ui/DisplayManager.h"
#include "ui/UIController.h"

constexpr const char* Application::INIT_STATE_NAMES_[];

Application::Application()
    : webServer_(80),
      logger_(systemState_.core.isTimeSynced),
      uiController_(logger_, &displayManager_, systemMetrics_, systemState_.pcMetrics,
                    systemState_.screen, colors_),
      networkManager_(logger_, httpClient_),
      displayManager_(display_, logger_),
      pcMetricsService_(networkManager_, systemMetrics_, logger_),
      taskManager_(logger_, uiController_, pcMetricsService_, systemState_.pcMetrics,
                   systemState_.core, systemState_.screen),
      httpServer_(uiController_, systemMetrics_),
      currentInitState_(InitState::INITIAL) {}

bool Application::initialize() {
    logger_.info("Application initialization started", true);

    while (!isTerminalState()) {
        if (!handleStateTransition()) {
            handleInitializationFailure();
            return false;
        }
    }

    logger_.info("Application initialization completed successfully", true);
    logger_.debugf("Free heap post-init: %d", ESP.getFreeHeap());
    uiController_.requestScreen(ScreenName::MAIN);
    systemState_.core.isInitialized = true;
    return true;
}

void Application::run() {
    if (!systemState_.core.isInitialized) {
        return;
    }

    if (Config::Watchdog::kEnableOnBoot) {
        esp_task_wdt_reset();
    }

    httpServer_.processRequests();
    vTaskDelay(Config::Timing::kMainLoopMs);
}

// Initialization Methods -----------------------------------------------------

bool Application::initializeDisplay() {
    logger_.info("Initializing display", true);
    displayManager_.initialize();
    systemState_.screen.isInitialized = true;
    uiController_.initialize();
    return true;
}

bool Application::initializeNetwork(uint8_t maxRetries) {
    logger_.info("Connecting to WiFi", true);
    return networkManager_.connect();
}

bool Application::initializeTimeService(uint8_t maxRetries) {
    logger_.info("Syncing time", true);

    for (uint8_t attempt = 1; attempt <= maxRetries; ++attempt) {
        if (ntpService_.syncTime()) {
            logger_.info("Time synchronized successfully", true);
            systemState_.core.isTimeSynced = true;
            return true;
        }

        logRetryAttempt("Time sync", attempt, maxRetries);
        delay(calculateBackoffDelay(attempt, Config::Init::kTimeSyncRetryDelayBaseMs));
    }

    logger_.warning("Time sync failed, using local time", true);
    return false;
}

bool Application::initializeWatchdog() {
    if (!Config::Watchdog::kEnableOnBoot) {
        logger_.info("Watchdog disabled in configuration", true);
        return true;
    }

    esp_err_t ret = esp_task_wdt_init(Config::Watchdog::kTimeoutMs / 1000, true);
    if (ret != ESP_OK) {
        logger_.error("Failed to initialize watchdog: " + String(esp_err_to_name(ret)),
                      true);
        return false;
    }

    ret = esp_task_wdt_add(nullptr);
    if (ret != ESP_OK) {
        logger_.error(
            "Failed to add main task to watchdog: " + String(esp_err_to_name(ret)), true);
        return false;
    }

    logger_.infof("Watchdog initialized with %dms timeout", Config::Watchdog::kTimeoutMs,
                  true);
    return true;
}

void Application::completeInitialization() {
    if (networkManager_.isConnected()) {
        httpServer_.begin();
        logger_.info("HTTP Server started", true);
    } else {
        logger_.warning("HTTP Server skipped: No network", true);
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
                logger_.error("Task creation failed", true);
                transitionTo(InitState::FAILED);
                return false;
            }
            transitionTo(InitState::NETWORK_INIT);
            return true;

        case InitState::NETWORK_INIT:
            if (!initializeNetwork(Config::Init::kDefaultNetworkRetries)) {
                logger_.warning("Network init failed, continuing", true);
            }
            transitionTo(InitState::TIME_INIT);
            return true;

        case InitState::TIME_INIT:
            if (!initializeTimeService(Config::Init::kDefaultTimeSyncRetries)) {
                logger_.warning("Time sync failed, continuing", true);
            }
            transitionTo(InitState::WATCHDOG_INIT);
            return true;

        case InitState::WATCHDOG_INIT:
            if (!initializeWatchdog()) {
                logger_.warning("Watchdog init failed, continuing", true);
            }
            transitionTo(InitState::FINAL_SETUP);
            return true;

        case InitState::FINAL_SETUP:
            completeInitialization();
            transitionTo(InitState::COMPLETE);
            return true;

        default:
            logger_.error("Unknown initialization state", true);
            transitionTo(InitState::FAILED);
            return false;
    }
}

void Application::transitionTo(InitState newState) {
    logger_.debug(getStateName(currentInitState_) + " -> " + getStateName(newState));
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
    // logger_.warningf("%s init attempt %d/%d failed", component, attempt, maxRetries);
}

uint16_t Application::calculateBackoffDelay(uint8_t attempt, uint16_t baseDelay) const {
    return baseDelay * (1 << (attempt - 1)) + (random(0, Config::Init::kBackoffJitterMs));
}

void Application::handleInitializationFailure() {
    logger_.critical("[FATAL] Application initialization failed in state: " +
                     getStateName(currentInitState_));
}