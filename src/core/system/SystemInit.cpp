#include "core/system/SystemInit.h"

SystemInit::SystemInit(ILogger &logger, SystemState::CoreState &coreState, LGFX &display,
                       ScreenManager &screenManager, NetworkManager &networkManager,
                       NtpService &ntpService, TaskManager &taskManager,
                       HttpServer &httpServer, SystemState::ScreenState &screenState)
    : logger_(logger),
      coreState_(coreState),
      display_(display),
      screenManager_(screenManager),
      networkManager_(networkManager),
      ntpService_(ntpService),
      taskManager_(taskManager),
      httpServer_(httpServer),
      screenState_(screenState),
      currentState_(State::INITIAL) {}

bool SystemInit::initializeAll() {
    logger_.info("System initialization started", true);
    bool success = true;

    while (currentState_ != State::COMPLETE && currentState_ != State::FAILED) {
        switch (currentState_) {
            case State::INITIAL:
                transitionTo(State::SERIAL_INIT);
                break;

            case State::SERIAL_INIT:
                if (initializeSerial()) {
                    transitionTo(State::DISPLAY_INIT);
                } else {
                    transitionTo(State::FAILED);
                    success = false;
                }
                break;

            case State::DISPLAY_INIT:
                if (initializeDisplay(Config::Init::kDefaultDisplayRetries)) {
                    transitionTo(State::TASKS_INIT);
                } else {
                    transitionTo(State::FAILED);
                    success = false;
                }
                break;

            case State::TASKS_INIT:
                if (taskManager_.createTasks()) {
                    transitionTo(State::NETWORK_INIT);
                } else {
                    logger_.error("Task creation failed", true);
                    transitionTo(State::FAILED);
                    success = false;
                }
                break;

            case State::NETWORK_INIT:
                if (initializeNetwork(Config::Init::kDefaultNetworkRetries)) {
                    transitionTo(State::TIME_INIT);
                } else {
                    logger_.warning("Network init failed, continuing", true);
                    transitionTo(State::TIME_INIT);  // Non-critical
                }
                break;

            case State::TIME_INIT:
                if (initializeTimeService(Config::Init::kDefaultTimeSyncRetries)) {
                    transitionTo(State::WATCHDOG_INIT);  // Transition to new state
                } else {
                    logger_.warning("Time sync failed, continuing", true);
                    transitionTo(State::WATCHDOG_INIT);  // Non-critical
                }
                break;

            case State::WATCHDOG_INIT:
                if (initializeWatchdog()) {
                    transitionTo(State::FINAL_SETUP);
                } else {
                    logger_.warning("Watchdog init failed, continuing", true);
                    transitionTo(State::FINAL_SETUP);  // Non-critical
                }
                break;

            case State::FINAL_SETUP:
                performFinalSetup();
                transitionTo(State::COMPLETE);
                break;

            default:
                logger_.error("Unknown initialization state", true);
                transitionTo(State::FAILED);
                success = false;
                break;
        }
    }

    logCompletionStatus(success);
    return success;
}

bool SystemInit::initializeSerial() {
    logger_.info("Serial start initialization", true);
    Serial.begin(Config::Debug::kSerialBaudRate);
    return true;
}

bool SystemInit::initializeDisplay(uint8_t maxRetries) {
    logger_.info("Initializing display", true);
    for (uint8_t attempt = 1; attempt <= maxRetries; ++attempt) {
        if (display_.init()) {
            postDisplayInitialization();
            screenState_.isInitialized = true;
            logger_.info("Display initialized successfully", true);
            return true;
        }
        logRetryAttempt("Display", attempt, maxRetries);
        delay(calculateBackoffDelay(attempt, Config::Init::kDisplayRetryDelayBaseMs));
    }
    logger_.error("Display initialization failed after all attempts", true);
    return false;
}

bool SystemInit::initializeNetwork(uint8_t maxRetries) {
    logger_.info("Connecting to WiFi", true);
    // return networkManager_.connect(maxRetries);  // TODO Assuming NetworkManager
    // handles its own retries
    return networkManager_.connect();  // Assuming NetworkManager handles its own retries
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

    // Initialize the watchdog with a simple timeout (compatible with older ESP-IDF)
    esp_err_t ret = esp_task_wdt_init(Config::Watchdog::kTimeoutMs / 1000,
                                      true);  // Timeout in seconds, enable panic
    if (ret != ESP_OK) {
        logger_.error("Failed to initialize watchdog: " + String(esp_err_to_name(ret)),
                      true);
        return false;
    }

    // Subscribe the current task (main task) to the watchdog
    ret = esp_task_wdt_add(nullptr);  // nullptr means current task
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

void SystemInit::postDisplayInitialization() {
    // display_.setBrightness(Config::Display::kInitialBrightness);
    display_.setBrightness(25);
    adjustDisplayOrientation();
    screenManager_.initialize();
}

void SystemInit::performFinalSetup() {
    if (networkManager_.isConnected()) {
        httpServer_.begin();
        logger_.info("HTTP Server started", true);
    } else {
        logger_.warning("HTTP Server skipped: No network", true);
    }
}

void SystemInit::transitionTo(State newState) {
    logger_.debug(getStateName(currentState_) + " -> " + getStateName(newState));
    currentState_ = newState;
}

void SystemInit::logCompletionStatus(bool success) {
    if (success) {
        logger_.info("System initialization completed successfully", true);
    } else {
        logger_.error(
            "System initialization failed at state: " + getStateName(currentState_),
            true);
    }
}

String SystemInit::getStateName(State state) {
    switch (state) {
        case State::INITIAL:
            return "INITIAL";
        case State::SERIAL_INIT:
            return "SERIAL_INIT";
        case State::DISPLAY_INIT:
            return "DISPLAY_INIT";
        case State::TASKS_INIT:
            return "TASKS_INIT";
        case State::NETWORK_INIT:
            return "NETWORK_INIT";
        case State::TIME_INIT:
            return "TIME_INIT";
        case State::WATCHDOG_INIT:
            return "WATCHDOG_INIT";
        case State::FINAL_SETUP:
            return "FINAL_SETUP";
        case State::COMPLETE:
            return "COMPLETE";
        case State::FAILED:
            return "FAILED";
        default:
            return "UNKNOWN";
    }
}

void SystemInit::logRetryAttempt(const char *component, uint8_t attempt,
                                 uint8_t maxRetries) {
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%s init attempt %d/%d failed", component, attempt,
             maxRetries);
    logger_.warning(buffer, true);
}

uint16_t SystemInit::calculateBackoffDelay(uint8_t attempt, uint16_t baseDelay) {
    return baseDelay * (1 << (attempt - 1)) + (random(0, Config::Init::kBackoffJitterMs));
}

void SystemInit::adjustDisplayOrientation() {
    if (display_.width() < display_.height()) {
        uint8_t currentRotation = display_.getRotation();
        display_.setRotation(currentRotation ^ 1);
    }
}