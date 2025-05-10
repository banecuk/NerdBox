#include "System.h"

// Add this definition for the static member
constexpr const char* System::INIT_STATE_NAMES_[];

System::System()
    : webServer_(80),
      logger_(systemState_.core.isTimeSynced),
      uiController_(logger_, &displayManager_, systemState_.pcMetrics, systemState_.screen),
      networkManager_(logger_, httpDownloader_),
      displayManager_(display_, logger_),
      pcMetricsService_(networkManager_),
      taskManager_(logger_, uiController_, pcMetricsService_, systemState_.pcMetrics,
                   systemState_.core, systemState_.screen),
      httpServer_(uiController_),
      currentInitState_(InitState::INITIAL) {}

bool System::initialize() {
    logger_.info("System initialization started", true);

    while (!isTerminalState()) {
        if (!handleStateTransition()) {
            handleInitializationFailure();
            return false;
        }
    }

    logger_.info("System initialization completed successfully", true);
    uiController_.requestScreen(ScreenName::MAIN);
    systemState_.core.isInitialized = true;
    return true;
}

void System::run() {
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

bool System::initializeSerial() {
    logger_.info("Initializing serial communication", true);
    Serial.begin(Config::Debug::kSerialBaudRate);
    return true;
}

bool System::initializeDisplay() {
    logger_.info("Initializing display", true);
    displayManager_.initialize();
    systemState_.screen.isInitialized = true;
    uiController_.initialize();
    return true;
}

bool System::initializeNetwork(uint8_t maxRetries) {
    logger_.info("Connecting to WiFi", true);
    return networkManager_.connect();
}

bool System::initializeTimeService(uint8_t maxRetries) {
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

bool System::initializeWatchdog() {
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

void System::completeInitialization() {
    if (networkManager_.isConnected()) {
        httpServer_.begin();
        logger_.info("HTTP Server started", true);
    } else {
        logger_.warning("HTTP Server skipped: No network", true);
    }
}

// State Management -----------------------------------------------------------

bool System::handleStateTransition() {
    switch (currentInitState_) {
        case InitState::INITIAL:
            transitionTo(InitState::SERIAL_INIT);
            return true;

        case InitState::SERIAL_INIT:
            return initializeSerial() ? transitionTo(InitState::DISPLAY_INIT),
                   true               : (transitionTo(InitState::FAILED), false);

        case InitState::DISPLAY_INIT:
            return initializeDisplay() ? transitionTo(InitState::TASKS_INIT),
                   true                : (transitionTo(InitState::FAILED), false);

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

void System::transitionTo(InitState newState) {
    logger_.debug(getStateName(currentInitState_) + " -> " + getStateName(newState));
    currentInitState_ = newState;
}

bool System::isTerminalState() const {
    return currentInitState_ == InitState::COMPLETE ||
           currentInitState_ == InitState::FAILED;
}

// Logging Helpers ------------------------------------------------------------

String System::getStateName(InitState state) const {
    return INIT_STATE_NAMES_[static_cast<int>(state)];
}

void System::logRetryAttempt(const char* component, uint8_t attempt,
                             uint8_t maxRetries) const {
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%s init attempt %d/%d failed", component, attempt,
             maxRetries);
    // logger_.warning(buffer, true);
}

uint16_t System::calculateBackoffDelay(uint8_t attempt, uint16_t baseDelay) const {
    return baseDelay * (1 << (attempt - 1)) + (random(0, Config::Init::kBackoffJitterMs));
}

void System::handleInitializationFailure() {
    logger_.critical("[FATAL] System initialization failed in state: " +
                     getStateName(currentInitState_));
}