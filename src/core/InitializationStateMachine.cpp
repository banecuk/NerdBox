#include "InitializationStateMachine.h"

#include "ApplicationComponents.h"
#include "ui/screens/ScreenTypes.h"

InitializationStateMachine::InitializationStateMachine(ApplicationComponents& components)
    : components_(components), currentState_(State::INITIAL) {}

bool InitializationStateMachine::initialize() {
    components_.logger.info("Initialization state machine started", true);

    while (!isTerminalState()) {
        bool success = false;

        switch (currentState_) {
            case State::INITIAL:
                success = handleInitial();
                break;
            case State::DISPLAY_INIT:
                success = handleDisplayInit();
                break;
            case State::TASKS_INIT:
                success = handleTasksInit();
                break;
            case State::NETWORK_INIT:
                success = handleNetworkInit();
                break;
            case State::TIME_INIT:
                success = handleTimeInit();
                break;
            case State::WATCHDOG_INIT:
                success = handleWatchdogInit();
                break;
            case State::FINAL_SETUP:
                success = handleFinalSetup();
                break;
            case State::COMPLETE:
                success = handleComplete();
                break;
            case State::FAILED:
                success = handleFailed();
                break;
        }

        if (!success) {
            components_.logger.critical("Initialization failed in state: " +
                                        getStateName(currentState_));
            return false;
        }
    }

    components_.logger.info("Initialization state machine completed successfully", true);
    return true;
}

bool InitializationStateMachine::isTerminalState() const {
    return currentState_ == State::COMPLETE || currentState_ == State::FAILED;
}

String InitializationStateMachine::getStateName(State state) const {
    return STATE_NAMES_[static_cast<int>(state)];
}

// State Handlers
bool InitializationStateMachine::handleInitial() {
    transitionTo(State::DISPLAY_INIT);
    return true;
}

bool InitializationStateMachine::handleDisplayInit() {
    components_.logger.info("Initializing display", true);
    components_.displayManager.initialize();
    components_.systemState.screen.isInitialized = true;
    components_.uiController.initialize();
    transitionTo(State::TASKS_INIT);
    return true;
}

bool InitializationStateMachine::handleTasksInit() {
    if (!components_.taskManager.createTasks()) {
        components_.logger.error("Task creation failed", true);
        transitionTo(State::FAILED);
        return false;
    }
    transitionTo(State::NETWORK_INIT);
    return true;
}

bool InitializationStateMachine::handleNetworkInit() {
    components_.logger.info("Connecting to WiFi", true);
    if (!components_.networkManager.connect()) {
        components_.logger.warning("Network init failed, continuing", true);
    }
    transitionTo(State::TIME_INIT);
    return true;
}

bool InitializationStateMachine::handleTimeInit() {
    components_.logger.info("Syncing time", true);

    for (uint8_t attempt = 1; attempt <= components_.config.getInitTimeSyncRetries(); ++attempt) {
        if (components_.ntpService.syncTime()) {
            components_.logger.info("Time synchronized successfully", true);
            components_.systemState.core.isTimeSynced = true;
            transitionTo(State::WATCHDOG_INIT);
            return true;
        }

        logRetryAttempt("Time sync", attempt, components_.config.getInitTimeSyncRetries());
        delay(calculateBackoffDelay(attempt, components_.config.getInitTimeSyncBaseDelayMs()));
    }

    components_.logger.warning("Time sync failed, using local time", true);
    transitionTo(State::WATCHDOG_INIT);
    return true;
}

bool InitializationStateMachine::handleWatchdogInit() {
    if (!components_.config.getWatchdogEnableOnBoot()) {
        components_.logger.info("Watchdog disabled in configuration", true);
        transitionTo(State::FINAL_SETUP);
        return true;
    }

    esp_err_t ret = esp_task_wdt_init(components_.config.getWatchdogTimeoutMs() / 1000, true);
    if (ret != ESP_OK) {
        components_.logger.error("Failed to initialize watchdog: " + String(esp_err_to_name(ret)),
                                 true);
        transitionTo(State::FINAL_SETUP);
        return true;
    }

    ret = esp_task_wdt_add(nullptr);
    if (ret != ESP_OK) {
        components_.logger.error(
            "Failed to add main task to watchdog: " + String(esp_err_to_name(ret)), true);
        transitionTo(State::FINAL_SETUP);
        return true;
    }

    components_.logger.infof("Watchdog initialized with %dms timeout",
                             components_.config.getWatchdogTimeoutMs(), true);
    transitionTo(State::FINAL_SETUP);
    return true;
}

bool InitializationStateMachine::handleFinalSetup() {
    if (components_.networkManager.isConnected()) {
        components_.webServerService.begin();
        components_.logger.info("HTTP Server started", true);
    } else {
        components_.logger.warning("HTTP Server skipped: No network", true);
    }

    components_.logger.debugf("Free heap post-init: %d", ESP.getFreeHeap());
    components_.uiController.requestScreen(ScreenName::MAIN);
    components_.systemState.core.isInitialized = true;

    transitionTo(State::COMPLETE);
    return true;
}

bool InitializationStateMachine::handleComplete() {
    // Terminal state - nothing to do
    return true;
}

bool InitializationStateMachine::handleFailed() {
    // Terminal state - nothing to do
    return false;
}

void InitializationStateMachine::transitionTo(State newState) {
    components_.logger.debug(getStateName(currentState_) + " -> " + getStateName(newState));
    currentState_ = newState;
}

void InitializationStateMachine::logRetryAttempt(const char* component, uint8_t attempt,
                                                 uint8_t maxRetries) const {
    // components_.logger.warningf("%s init attempt %d/%d failed", component, attempt,
    // maxRetries);
}

uint16_t InitializationStateMachine::calculateBackoffDelay(uint8_t attempt,
                                                           uint16_t baseDelay) const {
    return baseDelay * (1 << (attempt - 1)) +
           (random(0, components_.config.getInitBackoffJitterMs()));
}