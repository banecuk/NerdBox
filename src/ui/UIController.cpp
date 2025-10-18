#include "UIController.h"

#include "core/events/EventHandler.h"
#include "screens/BootScreen.h"
#include "screens/ScreenFactory.h"
#include "widgetScreens/MainScreen.h"
#include "widgetScreens/SettingsScreen.h"

UiController::UiController(DisplayContext& context, DisplayManager* displayManager,
                           ApplicationMetrics& systemMetrics, PcMetrics& pcMetrics,
                           SystemState::ScreenState& screenState, AppConfigInterface& config)
    : context_(context),
      logger_(context.getLogger()),
      displayManager_(displayManager),
      systemMetrics_(systemMetrics),
      pcMetrics_(pcMetrics),
      screenState_(screenState),
      config_(config),
      actionHandler_(std::make_unique<EventHandler>(this, context.getLogger())),
      touchManager_(
          std::make_unique<TouchManager>(context.getDisplay(), context.getLogger(), config)) {
    if (!displayManager_) {
        throw std::invalid_argument("[UiController] DisplayManager pointer cannot be null");
    }
    displayAccessMutex_ = xSemaphoreCreateMutex();
    if (!displayAccessMutex_) {
        throw std::runtime_error("[UiController] Failed to create display mutex");
    }
}

UiController::~UiController() {
    if (displayAccessMutex_) {
        vSemaphoreDelete(displayAccessMutex_);
    }
}

void UiController::initialize() {
    logger_.info("[UiController] Initializing UI");
    requestTransitionTo(ScreenName::BOOT);
}

bool UiController::requestTransitionTo(ScreenName screenName) {
    logger_.debugf("[UiController] Scheduling transition to screen %d, current=%d",
                   static_cast<int>(screenName), static_cast<int>(screenState_.activeScreen));

    if (screenName == ScreenName::NONE) {
        logger_.error("[UiController] Invalid screen: UNSET");
        return false;
    }

    if (screenName == screenState_.activeScreen && !activeTransition_.isActive) {
        logger_.debug("[UiController] Screen already active");
        return false;
    }

    activeTransition_.nextScreen = screenName;
    activeTransition_.phase = TransitionPhase::UNLOADING;
    activeTransition_.isActive = true;
    activeTransition_.startTime = millis();

    // Reset touch debounce timer during transitions to prevent accidental touches
    touchManager_->resetDebounce();

    return true;
}

void UiController::updateDisplay() {
    unsigned long startTime = millis();
    if (activeTransition_.isActive) {
        processTransitionPhase();

        if (millis() - activeTransition_.startTime > config_.getUiTransitionTimeoutMs()) {
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
    systemMetrics_.addScreenDrawTime(millis() - startTime);
}

bool UiController::tryAcquireDisplayLock() {
    const TickType_t timeout = pdMS_TO_TICKS(config_.getUiDisplayLockTimeoutMs());
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

// Transition lifecycle methods
void UiController::processTransitionPhase() {
    if (!tryAcquireDisplayLock()) {
        logger_.error("[UiController] Failed to acquire display lock");
        return;
    }

    displayManager_->getDisplay()->startWrite();
    logger_.debugf("[Heap] %d", ESP.getFreeHeap());
    logger_.debugf("[Stack] %u", uxTaskGetStackHighWaterMark(nullptr));

    switch (activeTransition_.phase) {
        case TransitionPhase::UNLOADING:
            logger_.debug("[UiController] Unloading current screen");
            unloadCurrentScreen();
            activeTransition_.phase = TransitionPhase::CLEARING;
            break;

        case TransitionPhase::CLEARING:
            logger_.debug("[UiController] Clearing display");
            clearDisplay();
            activeTransition_.phase = TransitionPhase::ACTIVATING;
            break;

        case TransitionPhase::ACTIVATING:
            logger_.debug("[UiController] Activating new screen");
            loadAndActivateScreen();
            completeTransition();
            break;

        case TransitionPhase::IDLE:
            logger_.error("[UiController] Unexpected IDLE state in transition");
            completeTransition();
            break;
    }

    displayManager_->getDisplay()->endWrite();
    releaseDisplayLock();
}

void UiController::unloadCurrentScreen() {
    if (currentScreen_) {
        currentScreen_->onExit();
        currentScreen_.reset();
    }
}

void UiController::clearDisplay() {
    if (displayManager_ && displayManager_->getDisplay()) {
        displayManager_->getDisplay()->fillScreen(TFT_BLACK);
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
    newScreen = ScreenFactory::createScreen(activeTransition_.nextScreen, logger_, displayManager_,
                                            pcMetrics_, this, config_);

    if (newScreen) {
        currentScreen_ = std::move(newScreen);
        screenState_.activeScreen = activeTransition_.nextScreen;
        currentScreen_->onEnter();
    } else {
        logger_.error("[UIController] Failed to create screen");
        requestTransitionTo(ScreenName::BOOT);  // Fallback
    }
}

void UiController::completeTransition() {
    activeTransition_.nextScreen = ScreenName::NONE;
    activeTransition_.phase = TransitionPhase::IDLE;
    activeTransition_.isActive = false;
    activeTransition_.startTime = 0;
}