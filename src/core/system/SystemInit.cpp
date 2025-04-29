#include "core/system/SystemInit.h"

SystemInit::SystemInit(ILogger& logger, SystemState::CoreState& coreState, LGFX& display,
                       DisplayManager& displayManager, ScreenManager& screenManager,
                       NetworkManager& networkManager, NtpService& ntpService,
                       TaskManager& taskManager, HttpServer& httpServer,
                       SystemState::ScreenState& screenState)
    : logger_(logger),
      coreState_(coreState),
      screenState_(screenState),
      display_(display),
      displayManager_(displayManager),
      screenManager_(screenManager),
      networkManager_(networkManager),
      ntpService_(ntpService),
      taskManager_(taskManager),
      httpServer_(httpServer),
      currentState_(State::INITIAL) {}

bool SystemInit::initializeAll() {
    logger_.info("System initialization started", true);

    while (!isTerminalState()) {
        if (!handleStateTransition()) {
            logCompletionStatus(false);
            return false;
        }
    }

    logCompletionStatus(true);
    return true;
}

bool SystemInit::initializeSerial() {
    logger_.info("Initializing serial communication", true);
    Serial.begin(Config::Debug::kSerialBaudRate);
    return true;
}

bool SystemInit::initializeDisplay() {
    logger_.info("Initializing display", true);
    displayManager_.initialize();
    screenState_.isInitialized = true;
    screenManager_.initialize();
    return true;
}

bool SystemInit::initializeNetwork(uint8_t maxRetries) {
    logger_.info("Connecting to WiFi", true);
    return networkManager_.connect();
}

bool SystemInit::initializeTimeService(uint8_t maxRetries) {
    logger_.info("Syncing time", true);

    for (uint8_t attempt = 1; attempt <= maxRetries; ++attempt) {
        if (ntpService_.syncTime()) {
            logger_.info("Time synchronized successfully", true);
            coreState_.isTimeSynced = true;
            return true;
        }

        logRetryAttempt("Time sync", attempt, maxRetries);
        delay(calculateBackoffDelay(attempt, Config::Init::kTimeSyncRetryDelayBaseMs));
    }

    logger_.warning("Time sync failed, using local time", true);
    return false;
}

bool SystemInit::initializeWatchdog() {
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

    logger_.info("Watchdog initialized with " + String(Config::Watchdog::kTimeoutMs) +
                     "ms timeout",
                 true);
    return true;
}

void SystemInit::performFinalSetup() {
    if (networkManager_.isConnected()) {
        httpServer_.begin();
        logger_.info("HTTP Server started", true);
    } else {
        logger_.warning("HTTP Server skipped: No network", true);
    }
}

bool SystemInit::handleStateTransition() {
    switch (currentState_) {
        case State::INITIAL:
            transitionTo(State::SERIAL_INIT);
            return true;

        case State::SERIAL_INIT:
            return initializeSerial() ? transitionTo(State::DISPLAY_INIT),
                   true               : (transitionTo(State::FAILED), false);

        case State::DISPLAY_INIT:
            return initializeDisplay() ? transitionTo(State::TASKS_INIT),
                   true                : (transitionTo(State::FAILED), false);

        case State::TASKS_INIT:
            if (!taskManager_.createTasks()) {
                logger_.error("Task creation failed", true);
                transitionTo(State::FAILED);
                return false;
            }
            transitionTo(State::NETWORK_INIT);
            return true;

        case State::NETWORK_INIT:
            if (!initializeNetwork(Config::Init::kDefaultNetworkRetries)) {
                logger_.warning("Network init failed, continuing", true);
            }
            transitionTo(State::TIME_INIT);
            return true;

        case State::TIME_INIT:
            if (!initializeTimeService(Config::Init::kDefaultTimeSyncRetries)) {
                logger_.warning("Time sync failed, continuing", true);
            }
            transitionTo(State::WATCHDOG_INIT);
            return true;

        case State::WATCHDOG_INIT:
            if (!initializeWatchdog()) {
                logger_.warning("Watchdog init failed, continuing", true);
            }
            transitionTo(State::FINAL_SETUP);
            return true;

        case State::FINAL_SETUP:
            performFinalSetup();
            transitionTo(State::COMPLETE);
            return true;

        default:
            logger_.error("Unknown initialization state", true);
            transitionTo(State::FAILED);
            return false;
    }
}

void SystemInit::transitionTo(State newState) {
    logger_.debug(getStateName(currentState_) + " -> " + getStateName(newState));
    currentState_ = newState;
}

bool SystemInit::isTerminalState() const {
    return currentState_ == State::COMPLETE || currentState_ == State::FAILED;
}

void SystemInit::logCompletionStatus(bool success) const {
    if (success) {
        logger_.info("System initialization completed successfully", true);
    } else {
        logger_.error(
            "System initialization failed at state: " + getStateName(currentState_),
            true);
    }
}

void SystemInit::logRetryAttempt(const char* component, uint8_t attempt,
                                 uint8_t maxRetries) const {
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%s init attempt %d/%d failed", component, attempt,
             maxRetries);
    logger_.warning(buffer, true);
}

uint16_t SystemInit::calculateBackoffDelay(uint8_t attempt, uint16_t baseDelay) const {
    return baseDelay * (1 << (attempt - 1)) + (random(0, Config::Init::kBackoffJitterMs));
}