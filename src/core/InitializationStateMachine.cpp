#include "InitializationStateMachine.h"

#include "ui/screens/ScreenTypes.h"

InitializationStateMachine::InitializationStateMachine(IInitializationTarget& target)
    : target_(target), currentState_(State::INITIAL) {}

bool InitializationStateMachine::initialize() {
    target_.logger().info("Initialization state machine started", true);

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
            target_.logger().criticalf("Initialization failed in state: %s",
                                       getStateName(currentState_));
            return false;
        }
    }

    target_.logger().info("Initialization state machine completed successfully", true);
    return true;
}

bool InitializationStateMachine::isTerminalState() const {
    return currentState_ == State::COMPLETE || currentState_ == State::FAILED;
}

const char* InitializationStateMachine::getStateName(State state) const {
    return STATE_NAMES_[static_cast<int>(state)];
}

// ---------------------------------------------------------------------------
// State handlers
// ---------------------------------------------------------------------------

bool InitializationStateMachine::handleInitial() {
    transitionTo(State::DISPLAY_INIT);
    return true;
}

bool InitializationStateMachine::handleDisplayInit() {
    target_.logger().info("Initializing display", true);
    target_.initializeDisplay();
    target_.setScreenInitialized();
    target_.initializeUi();
    transitionTo(State::TASKS_INIT);
    return true;
}

bool InitializationStateMachine::handleTasksInit() {
    if (!target_.createTasks()) {
        target_.logger().error("Task creation failed", true);
        transitionTo(State::FAILED);
        return false;
    }
    transitionTo(State::NETWORK_INIT);
    return true;
}

bool InitializationStateMachine::handleNetworkInit() {
    target_.logger().info("Connecting to WiFi", true);
    if (!target_.connectNetwork()) {
        target_.logger().warning("Network init failed, continuing", true);
    }
    transitionTo(State::TIME_INIT);
    return true;
}

bool InitializationStateMachine::handleTimeInit() {
    target_.logger().info("Syncing time", true);

    for (uint8_t attempt = 1; attempt <= target_.initTimeSyncRetries(); ++attempt) {
        if (target_.syncTime()) {
            target_.logger().info("Time synchronized successfully", true);
            target_.setTimeSynced();
            transitionTo(State::WATCHDOG_INIT);
            return true;
        }

        delay(calculateBackoffDelay(attempt, target_.initTimeSyncBaseDelayMs()));
    }

    target_.logger().warning("Time sync failed, using local time", true);
    transitionTo(State::WATCHDOG_INIT);
    return true;
}

bool InitializationStateMachine::handleWatchdogInit() {
    if (!target_.watchdogEnabledOnBoot()) {
        target_.logger().info("Watchdog disabled in configuration", true);
        transitionTo(State::FINAL_SETUP);
        return true;
    }

    esp_err_t ret = esp_task_wdt_init(target_.watchdogTimeoutMs() / 1000, true);
    if (ret != ESP_OK) {
        target_.logger().errorf("Failed to initialize watchdog: %s", esp_err_to_name(ret));
        transitionTo(State::FINAL_SETUP);
        return true;
    }

    ret = esp_task_wdt_add(nullptr);
    if (ret != ESP_OK) {
        target_.logger().errorf("Failed to add main task to watchdog: %s", esp_err_to_name(ret));
        transitionTo(State::FINAL_SETUP);
        return true;
    }

    target_.logger().infof("Watchdog initialized with %dms timeout", target_.watchdogTimeoutMs());
    transitionTo(State::FINAL_SETUP);
    return true;
}

bool InitializationStateMachine::handleFinalSetup() {
    if (target_.isNetworkConnected()) {
        target_.beginWebServer();
        target_.logger().info("HTTP Server started", true);
    } else {
        target_.logger().warning("HTTP Server skipped: No network", true);
    }

    target_.postInitializeDisplay();

    target_.logger().debugf("Free heap post-init: %d", ESP.getFreeHeap());
    target_.requestScreen(ScreenName::MAIN);
    target_.setSystemInitialized();

    transitionTo(State::COMPLETE);
    return true;
}

bool InitializationStateMachine::handleComplete() {
    return true;  // Terminal state — nothing to do
}

bool InitializationStateMachine::handleFailed() {
    return false;  // Terminal state — nothing to do
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

void InitializationStateMachine::transitionTo(State newState) {
    target_.logger().debugf("%s -> %s", getStateName(currentState_), getStateName(newState));
    currentState_ = newState;
}

uint16_t InitializationStateMachine::calculateBackoffDelay(uint8_t attempt,
                                                           uint16_t baseDelay) const {
    return baseDelay * (1 << (attempt - 1)) + (random(0, target_.initBackoffJitterMs()));
}
