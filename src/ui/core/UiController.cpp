#include "UiController.h"

#include "ui/core/UiEventHandler.h"
#include "ui/screens/BootScreen.h"
#include "ui/screens/MainScreen.h"
#include "ui/screens/ScreenFactory.h"
#include "ui/screens/SettingsScreen.h"
#include "utils/logging/LogMacros.h"

UiController::UiController(DisplayContext& context, DisplayManager& displayManager,
                           ApplicationMetrics& systemMetrics, PcMetrics& pcMetrics,
                           SystemState::ScreenState& screenState, const AppSettings& config,
                           NetworkManager& networkManager, const AirQualityData& airQualityData,
                           const NetworkStatus& netStatus, WeatherData& weatherData)
    : logger_(context.getLogger()),
      displayManager_(displayManager),
      context_(context),
      systemMetrics_(systemMetrics),
      pcMetrics_(pcMetrics),
      screenState_(screenState),
      config_(config),
      networkManager_(networkManager),
      airQualityData_(airQualityData),
      netStatus_(netStatus),
      weatherData_(weatherData),
      actionHandler_(std::make_unique<UiEventHandler>(this, context.getLogger())),
      touchManager_(
          std::make_unique<TouchManager>(context.getDisplay(), context.getLogger(), config)),
      displayAccessMutex_(xSemaphoreCreateMutex()) {
    if (!displayAccessMutex_) {
        // Boot-fatal: initialize() reports this to InitializationStateMachine,
        // which transitions to FAILED instead of pressing on with a null
        // mutex. No exception — this runs inside ApplicationComponents'
        // member construction, before any handler exists to catch one.
        logger_.critical("[UiController] Failed to create display mutex");
    }
}

UiController::~UiController() {
    if (displayAccessMutex_) {
        vSemaphoreDelete(displayAccessMutex_);
    }
}

bool UiController::initialize() {
    if (!displayAccessMutex_) {
        logger_.critical("[UiController] Cannot initialize UI: display mutex unavailable");
        return false;
    }
    logger_.info("[UiController] Initializing UI");
    requestTransitionTo(ScreenName::BOOT);
    return true;
}

bool UiController::requestTransitionTo(ScreenName screenName) {
    if (screenName == ScreenName::NONE) {
        logger_.error("[UiController] Invalid screen: UNSET");
        return false;
    }

    LOG_DEBUGF(logger_, "[UiController] Scheduling transition to screen %d",
               static_cast<int>(screenName));

    // May run on a different task than updateDisplay() — see the header
    // comment on pendingScreen_. Do not touch activeTransition_/currentScreen_/
    // screenState_ here.
    pendingScreen_.store(screenName, std::memory_order_release);
    return true;
}

void UiController::updateDisplay() {
    unsigned long startTime = millis();
    // Separate from startTime above: that one feeds the transition timeout
    // (compared against millis() elsewhere), this one measures the frame
    // itself at microsecond resolution — a 16 ms frame in whole milliseconds
    // quantizes to 0 or 1, which made the reported average meaningless.
    const uint32_t drawStartUs = micros();

    // Drain any cross-task request before touching activeTransition_ — this
    // is the only place pendingScreen_ is read, and the only place
    // activeTransition_ is written, so the two never race.
    const ScreenName requested =
        pendingScreen_.exchange(ScreenName::NONE, std::memory_order_acquire);
    if (requested != ScreenName::NONE &&
        !(requested == screenState_.activeScreen && !activeTransition_.isActive)) {
        activeTransition_.nextScreen = requested;
        activeTransition_.phase = TransitionPhase::UNLOADING;
        activeTransition_.isActive = true;
        activeTransition_.startTime = startTime;
    }

    if (activeTransition_.isActive) {
        processTransitionPhase();

        if (millis() - activeTransition_.startTime > config_.uiTransitionTimeoutMs) {
            logger_.error("[UiController] Transition timeout, resetting");
            completeTransition();
        }
    } else if (currentScreen_) {
        currentScreen_->draw();
        processTouchInput();
    } else {
        logger_.warning("[UiController] No screen to draw");
        requestTransitionTo(ScreenName::BOOT);  // Fallback to boot screen
    }
    systemMetrics_.addScreenDrawTimeUs(micros() - drawStartUs);
}

bool UiController::tryAcquireDisplayLock() {
    const TickType_t timeout = pdMS_TO_TICKS(config_.uiDisplayLockTimeoutMs);
    BaseType_t res = xSemaphoreTake(displayAccessMutex_, timeout);
    if (res != pdTRUE) {
        logger_.error("[UiController] Display lock timeout");
        return false;
    }
    return true;
}

void UiController::releaseDisplayLock() {
    xSemaphoreGive(displayAccessMutex_);
}

void UiController::processTransitionPhase() {
    if (!tryAcquireDisplayLock()) {
        logger_.error("[UiController] Failed to acquire display lock");
        return;
    }

    displayManager_.getDisplay()->startWrite();
    switch (activeTransition_.phase) {
        case TransitionPhase::UNLOADING:
            LOG_DEBUG(logger_, "[UiController] Unloading current screen");
            LOG_DEBUGF(logger_, "[Heap] %d", ESP.getFreeHeap());
            LOG_DEBUGF(logger_, "[Stack] %u", uxTaskGetStackHighWaterMark(nullptr));
            unloadCurrentScreen();
            activeTransition_.phase = TransitionPhase::CLEARING;
            break;

        case TransitionPhase::CLEARING:
            LOG_DEBUG(logger_, "[UiController] Clearing display");
            clearDisplay();
            activeTransition_.phase = TransitionPhase::ACTIVATING;
            break;

        case TransitionPhase::ACTIVATING:
            LOG_DEBUG(logger_, "[UiController] Activating new screen");
            loadAndActivateScreen();
            completeTransition();
            break;

        case TransitionPhase::IDLE:
            logger_.error("[UiController] Unexpected IDLE state in transition");
            completeTransition();
            break;
    }

    displayManager_.getDisplay()->endWrite();
    releaseDisplayLock();
}

void UiController::unloadCurrentScreen() {
    if (currentScreen_) {
        currentScreen_->onExit();
        currentScreen_.reset();
    }
}

void UiController::clearDisplay() {
    if (displayManager_.getDisplay()) {
        displayManager_.getDisplay()->fillScreen(TFT_BLACK);
    } else {
        logger_.error("[UiController] Invalid display driver");
    }
}

void UiController::loadAndActivateScreen() {
    if (activeTransition_.nextScreen == ScreenName::NONE) {
        logger_.error("[UiController] No screen to activate");
        completeTransition();
        return;
    }

    std::unique_ptr<ScreenInterface> newScreen;
    ScreenCreationContext ctx{logger_,
                              context_.getScreenLogQueue(),
                              &displayManager_,
                              pcMetrics_,
                              this,
                              config_,
                              systemMetrics_,
                              networkManager_,
                              airQualityData_,
                              netStatus_,
                              weatherData_};
    newScreen = ScreenFactory::createScreen(activeTransition_.nextScreen, ctx);

    if (newScreen) {
        currentScreen_ = std::move(newScreen);
        screenState_.activeScreen = activeTransition_.nextScreen;

        // Weather screen: fetch on entry rather than waiting for WeatherJob's
        // own 2h background refresh, so tapping in always shows current
        // conditions. WeatherJob backs off on failure, so this one-shot
        // request doesn't turn into aggressive polling — reuses the same
        // flag as its midnight-rollover refresh.
        if (activeTransition_.nextScreen == ScreenName::WEATHER) {
            weatherData_.refreshRequested.store(true);
        }

        currentScreen_->onEnter();
    } else {
        logger_.error("[UiController] Failed to create screen");
        requestTransitionTo(ScreenName::BOOT);  // Fallback
    }
}

void UiController::completeTransition() {
    activeTransition_.nextScreen = ScreenName::NONE;
    activeTransition_.phase = TransitionPhase::IDLE;
    activeTransition_.isActive = false;
    activeTransition_.startTime = 0;

    // Suppress touches briefly to prevent accidental input / rapid re-triggering
    // right after a screen transition completes.
    touchManager_->suppressFor(config_.uiScreenTransitionCooldownMs);
    LOG_DEBUG(logger_, "[UiController] Screen transition complete - cooldown active");
}

void UiController::processTouchInput() {
    // Use TouchManager to read and validate touch
    TouchManager::TouchPoint touch = touchManager_->readTouch();

    if (!touch.valid) {
        return;  // No valid touch detected
    }

    // Pass valid touch to current screen
    if (currentScreen_) {
        currentScreen_->handleTouch(touch.x, touch.y);
    } else {
        logger_.warning("[UiController] No screen to handle touch");
    }
}