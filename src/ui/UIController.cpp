#include "UIController.h"

#include "core/events/EventHandler.h"
#include "screens/BootScreen.h"
#include "screens/ScreenFactory.h"
#include "widgetScreens/MainScreen.h"
#include "widgetScreens/SettingsScreen.h"

UIController::UIController(DisplayContext& context, DisplayManager* displayManager,
                           ApplicationMetrics& systemMetrics, PcMetrics& pcMetrics,
                           SystemState::ScreenState& screenState,
                           AppConfigInterface& config)
    : context_(context),
      logger_(context.getLogger()),
      displayManager_(displayManager),
      systemMetrics_(systemMetrics),
      pcMetrics_(pcMetrics),
      screenState_(screenState),
      config_(config),
      actionHandler_(std::make_unique<EventHandler>(this, context.getLogger())) {
    if (!displayManager_) {
        throw std::invalid_argument(
            "[UIController] DisplayManager pointer cannot be null");
    }
    displayAccessMutex_ = xSemaphoreCreateMutex();
    if (!displayAccessMutex_) {
        throw std::runtime_error("[UIController] Failed to create display mutex");
    }
}

UIController::~UIController() {
    if (displayAccessMutex_) {
        vSemaphoreDelete(displayAccessMutex_);
    }
}

void UIController::initialize() {
    logger_.info("[UIController] Initializing UI");
    requestTransitionTo(ScreenName::BOOT);
}

bool UIController::requestTransitionTo(ScreenName screenName) {
    logger_.debugf("[UIController] Scheduling transition to screen %d, current=%d",
                   static_cast<int>(screenName),
                   static_cast<int>(screenState_.activeScreen));

    if (screenName == ScreenName::UNSET) {
        logger_.error("[UIController] Invalid screen: UNSET");
        return false;
    }

    if (screenName == screenState_.activeScreen && !activeTransition_.isActive) {
        logger_.debug("[UIController] Screen already active");
        return false;
    }

    activeTransition_.nextScreen = screenName;
    activeTransition_.phase = TransitionPhase::UNLOADING;
    activeTransition_.isActive = true;
    activeTransition_.startTime = millis();
    return true;
}

void UIController::updateDisplay() {
    unsigned long startTime = millis();
    if (activeTransition_.isActive) {
        processTransitionPhase();

        if (millis() - activeTransition_.startTime > config_.getUiTransitionTimeoutMs()) {
            logger_.error("[UIController] Transition timeout, resetting");
            completeTransition();
        }
    } else if (currentScreen_) {
        currentScreen_->draw();
        processTouchInput();
    } else {
        logger_.warning("[UIController] No screen to draw");
        requestTransitionTo(ScreenName::BOOT);  // Fallback to boot screen
    }
    systemMetrics_.addScreenDrawTime(millis() - startTime);
}

bool UIController::tryAcquireDisplayLock() {
    const TickType_t timeout = pdMS_TO_TICKS(config_.getUiDisplayLockTimeoutMs());
    BaseType_t res = xSemaphoreTake(displayAccessMutex_, timeout);
    if (res != pdTRUE) {
        logger_.error("[UIController] Display lock timeout");
        return false;
    }
    return true;
}

void UIController::releaseDisplayLock() { xSemaphoreGive(displayAccessMutex_); }

// Transition lifecycle methods
void UIController::processTransitionPhase() {
    if (!tryAcquireDisplayLock()) {
        logger_.error("[UIController] Failed to acquire display lock");
        return;
    }

    displayManager_->getDisplay()->startWrite();
    logger_.debugf("[Heap] %d", ESP.getFreeHeap());
    logger_.debugf("[Stack] %u", uxTaskGetStackHighWaterMark(nullptr));

    switch (activeTransition_.phase) {
        case TransitionPhase::UNLOADING:
            logger_.debug("[UIController] Unloading current screen");
            unloadCurrentScreen();
            activeTransition_.phase = TransitionPhase::CLEARING;
            break;

        case TransitionPhase::CLEARING:
            logger_.debug("[UIController] Clearing display");
            clearDisplay();
            activeTransition_.phase = TransitionPhase::ACTIVATING;
            break;

        case TransitionPhase::ACTIVATING:
            logger_.debug("[UIController] Activating new screen");
            loadAndActivateScreen();
            completeTransition();
            break;

        case TransitionPhase::IDLE:
            logger_.error("[UIController] Unexpected IDLE state in transition");
            completeTransition();
            break;
    }

    displayManager_->getDisplay()->endWrite();
    releaseDisplayLock();
}

void UIController::unloadCurrentScreen() {
    if (currentScreen_) {
        currentScreen_->onExit();
        currentScreen_.reset();
    }
}

void UIController::clearDisplay() {
    if (displayManager_ && displayManager_->getDisplay()) {
        displayManager_->getDisplay()->fillScreen(TFT_BLACK);
    } else {
        logger_.error("[UIController] Invalid display driver");
    }
}

void UIController::loadAndActivateScreen() {
    if (activeTransition_.nextScreen == ScreenName::UNSET) {
        logger_.error("[UIController] No screen to activate");
        completeTransition();
        return;
    }

    std::unique_ptr<ScreenInterface> newScreen;
    newScreen = ScreenFactory::createScreen(activeTransition_.nextScreen, logger_,
                                            displayManager_, pcMetrics_, this, config_);

    if (newScreen) {
        currentScreen_ = std::move(newScreen);
        screenState_.activeScreen = activeTransition_.nextScreen;
        currentScreen_->onEnter();
    } else {
        logger_.error("[UIController] Failed to create screen");
        requestTransitionTo(ScreenName::BOOT);  // Fallback
    }
}

void UIController::completeTransition() {
    activeTransition_.nextScreen = ScreenName::UNSET;
    activeTransition_.phase = TransitionPhase::IDLE;
    activeTransition_.isActive = false;
    activeTransition_.startTime = 0;
}