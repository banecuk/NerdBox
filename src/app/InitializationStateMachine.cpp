#include "InitializationStateMachine.h"

#include <algorithm>

#include "core/ScreenTypes.h"
#include "utils/logging/LogMacros.h"

InitializationStateMachine::InitializationStateMachine(IInitializationTarget& target,
                                                       const AppSettings& config)
    : target_(target), config_(config) {
    static_assert(stepsAreOrdered(),
                  "kSteps[] must list every step in State order, with strictly increasing "
                  "progressPercent below 100");
}

bool InitializationStateMachine::initialize() {
    if (currentState_ == State::COMPLETE)
        return true;
    if (currentState_ == State::FAILED)
        return false;

    target_.logger().info("Initialization state machine started", true);

    for (const Step& step : kSteps) {
        transitionTo(step.state);
        if (!(this->*step.run)()) {
            target_.logger().criticalf("Initialization failed in state: %s", step.name);
            transitionTo(State::FAILED);
            return false;
        }
    }

    transitionTo(State::COMPLETE);
    target_.logger().info("Initialization state machine completed successfully", true);
    return true;
}

uint8_t InitializationStateMachine::progressPercent() const {
    switch (currentState_) {
        case State::INITIAL:
            return 0;
        case State::COMPLETE:
            return 100;
        case State::FAILED:
            return 0;
        default:
            return kSteps[static_cast<size_t>(currentState_)].progressPercent;
    }
}

const char* InitializationStateMachine::getStateName(State state) {
    switch (state) {
        case State::INITIAL:
            return "INITIAL";
        case State::COMPLETE:
            return "COMPLETE";
        case State::FAILED:
            return "FAILED";
        default:
            return kSteps[static_cast<size_t>(state)].name;
    }
}

// ---------------------------------------------------------------------------
// Steps — run in kSteps[] order
// ---------------------------------------------------------------------------

bool InitializationStateMachine::stepDisplayInit() {
    target_.logger().info("Initializing display", true);
    target_.initializeDisplay();
    if (!target_.initializeUi()) {
        target_.logger().critical("UI initialization failed", true);
        return false;
    }
    return true;
}

bool InitializationStateMachine::stepWatchdogInit() {
    if (!config_.watchdogEnableOnBoot) {
        target_.logger().info("Watchdog disabled in configuration", true);
        return true;
    }

    // Only the watchdog itself is armed here, before any task is created —
    // that ordering is what TASKS_INIT needs to safely add the
    // screen/background tasks. The main/setup task itself joins later, in
    // addMainTaskToWatchdog() — see its declaration for why.
    //
    // A failure is not boot-fatal, but it is recorded by the target
    // (TaskManager) so nothing later adds tasks to / resets an unarmed
    // watchdog or reports it as active (B6).
    if (!target_.initWatchdog(static_cast<uint32_t>(config_.watchdogTimeoutMs))) {
        target_.logger().critical("Watchdog could not be armed, running without it", true);
        return true;
    }

    target_.logger().infof("Watchdog initialized with %lums timeout", config_.watchdogTimeoutMs);
    return true;
}

bool InitializationStateMachine::stepTasksInit() {
    if (!target_.createTasks()) {
        target_.logger().error("Task creation failed", true);
        return false;
    }

    // The screen task only draws once isInitialized is set. Set it now, right
    // after the task exists, so it can actually process the BOOT transition
    // requested back in DISPLAY_INIT and render the log lines queued by the
    // network/time steps below. Setting this later (e.g. alongside
    // requestScreen(MAIN) in FINAL_SETUP) means the BOOT transition never
    // gets processed before MAIN supersedes it — the boot screen is skipped
    // entirely.
    target_.setScreenInitialized();
    return true;
}

bool InitializationStateMachine::stepNetworkInit() {
    target_.logger().info("Connecting to WiFi", true);
    if (!target_.connectNetwork()) {
        target_.logger().warning("Network init failed, continuing", true);
    }
    return true;
}

bool InitializationStateMachine::stepTimeInit() {
    target_.logger().info("Syncing time", true);

    const uint8_t maxRetries = config_.initTimeSyncRetries;
    for (uint8_t attempt = 1; attempt <= maxRetries; ++attempt) {
        if (target_.syncTime()) {
            target_.logger().info("Time synchronized successfully", true);
            addMainTaskToWatchdog();
            return true;
        }

        // Skip the backoff after the final attempt — there's nothing left to
        // wait for before falling through to "give up and use local time".
        if (attempt < maxRetries) {
            const uint32_t jitter = target_.randomBelow(config_.initBackoffJitterMs);
            target_.delayMs(backoffDelayMs(attempt, config_.initTimeSyncBaseDelayMs, jitter));
        }
    }

    target_.logger().warning("Time sync failed, using local time", true);
    addMainTaskToWatchdog();
    return true;
}

bool InitializationStateMachine::stepFinalSetup() {
    // Always start the server, even without WiFi: WebServer listens on
    // 0.0.0.0, so it starts answering as soon as WifiReconnectJob brings the
    // link up later. Skipping it here (the old behaviour) left the device
    // with no /api, /metrics, /restart or POST /audio until the next reboot
    // whenever it booted before the router (B37).
    target_.beginWebServer();
    if (target_.isNetworkConnected()) {
        target_.logger().info("HTTP Server started", true);
    } else {
        target_.logger().warning("HTTP Server started (no network yet)", true);
    }

    target_.postInitializeDisplay();

    LOG_DEBUGF(target_.logger(), "Free heap post-init: %u", target_.freeHeap());
    target_.requestScreen(ScreenName::MAIN);
    target_.setSystemInitialized();
    return true;
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

void InitializationStateMachine::addMainTaskToWatchdog() {
    if (!config_.watchdogEnableOnBoot)
        return;

    // The target logs its own failure (and no-ops if the watchdog was never
    // armed); nothing more to do here either way.
    if (target_.addCurrentTaskToWatchdog()) {
        LOG_DEBUG(target_.logger(), "Main task added to watchdog", true);
    }
}

void InitializationStateMachine::transitionTo(State newState) {
    LOG_DEBUGF(target_.logger(), "%s -> %s", getStateName(currentState_), getStateName(newState));
    currentState_ = newState;
}

uint32_t InitializationStateMachine::backoffDelayMs(uint8_t attempt, uint32_t baseDelayMs,
                                                    uint32_t jitter) {
    // Base delay, result and the intermediate math are all 32/64-bit and the
    // clamp happens last, so raising kTimeSyncRetryDelayBaseMs or the retry
    // count can never wrap the backoff to a *shorter* delay than the previous
    // attempt (B16, B62 — the parameter used to be uint16_t, narrowing the
    // base delay before any clamp ran). Today's max is 450ms.
    const uint8_t shift = (attempt == 0) ? 0 : (attempt - 1 > 31 ? 31 : attempt - 1);
    const uint64_t exponential = static_cast<uint64_t>(baseDelayMs) << shift;
    const uint64_t delay = exponential + jitter;

    return static_cast<uint32_t>(std::min<uint64_t>(delay, kMaxBackoffDelayMs));
}
