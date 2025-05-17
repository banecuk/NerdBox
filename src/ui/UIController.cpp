#include "UIController.h"

#include <esp_task_wdt.h>

#include "core/events/EventHandler.h"
#include "screens/BootScreen.h"
#include "screens/ScreenFactory.h"
#include "widgetScreens/MainScreen.h"
#include "widgetScreens/SettingsScreen.h"

UIController::UIController(LoggerInterface& logger, DisplayManager* displayManager,
                           PcMetrics& pcMetrics, SystemState::ScreenState& screenState)
    : logger_(logger),
      displayManager_(displayManager),
      pcMetrics_(pcMetrics),
      screenState_(screenState),
      actionHandler_(std::make_unique<EventHandler>(this, logger)) {
    if (!displayManager_) {
        throw std::invalid_argument(
            "[UIController] DisplayManager pointer cannot be null");
    }
    displayMutex_ = xSemaphoreCreateMutex();
    if (!displayMutex_) {
        throw std::runtime_error("[UIController] Failed to create display mutex");
    }
}

UIController::~UIController() {
    if (displayMutex_) {
        vSemaphoreDelete(displayMutex_);
    }
}

void UIController::initialize() {
    logger_.info("[UIController] Initializing UI");
    requestsScreenTransition(ScreenName::BOOT);
}

bool UIController::requestsScreenTransition(ScreenName screenName) {
    logger_.debugf("[UIController] Scheduling transition to screen %d, current=%d",
                   static_cast<int>(screenName),
                   static_cast<int>(screenState_.activeScreen));

    if (screenName == ScreenName::UNSET) {
        logger_.error("[UIController] Invalid screen: UNSET");
        return false;
    }

    if (screenName == screenState_.activeScreen && !transition_.isActive) {
        logger_.debug("[UIController] Screen already active");
        return false;
    }

    transition_.nextScreen = screenName;
    transition_.state = ScreenTransitionState::UNLOADING;
    transition_.isActive = true;
    transition_.startTime = millis();
    // logger_.debugf("[UIController] Transition scheduled: nextScreen=%d, state=%d",
    //                static_cast<int>(transition_.nextScreen),
    //                static_cast<int>(transition_.state));
    return true;
}

void UIController::updateDisplay() {
    if (transition_.isActive) {
        // logger_.debug("[UIController] Processing screen transition");
        handleScreenTransition();

        if (millis() - transition_.startTime > 1000) {
            logger_.error("[UIController] Transition timeout, resetting");
            resetTransitionState();
        }
    } else if (currentScreen_) {
        currentScreen_->draw();
        processTouchInput();
    } else {
        logger_.warning("[UIController] No screen to draw");
        requestsScreenTransition(ScreenName::BOOT);  // Fallback to boot screen
    }
}

void UIController::handleScreenTransition() {
    // logger_.debugf("[UIController] Transition state=%d",
    //                static_cast<int>(transition_.state));
    if (!tryAcquireDisplayLock()) {
        logger_.error("[UIController] Failed to acquire display lock");
        return;
    }

    displayManager_->getDisplay()->startWrite();
    logger_.debugf("[Heap] %d", ESP.getFreeHeap());
    logger_.debugf("[Stack] %u", uxTaskGetStackHighWaterMark(nullptr));

    switch (transition_.state) {
        case ScreenTransitionState::UNLOADING:
            logger_.debug("[UIController] Unloading current screen");
            unloadCurrentScreen();
            transition_.state = ScreenTransitionState::CLEARING;
            break;

        case ScreenTransitionState::CLEARING:
            logger_.debug("[UIController] Clearing display");
            clearDisplay();
            transition_.state = ScreenTransitionState::ACTIVATING;
            break;

        case ScreenTransitionState::ACTIVATING:
            logger_.debug("[UIController] Activating new screen");
            activateNextScreen();
            resetTransitionState();
            break;

        case ScreenTransitionState::IDLE:
            logger_.error("[UIController] Unexpected IDLE state in transition");
            resetTransitionState();
            break;
    }

    displayManager_->getDisplay()->endWrite();
    releaseDisplayLock();
}

void UIController::unloadCurrentScreen() {
    if (currentScreen_) {
        // logger_.debug("[UIController] Unloading screen");
        currentScreen_->onExit();
        currentScreen_.reset();
    }
}

void UIController::clearDisplay() {
    if (displayManager_ && displayManager_->getDisplay()) {
        // logger_.debug("[UIController] Clearing display");
        displayManager_->getDisplay()->fillScreen(TFT_BLACK);
    } else {
        logger_.error("[UIController] Invalid display driver");
    }
}

void UIController::activateNextScreen() {
    if (transition_.nextScreen == ScreenName::UNSET) {
        logger_.error("[UIController] No screen to activate");
        resetTransitionState();
        return;
    }

    // logger_.debugf("[UIController] Loading screen %d",
    //                static_cast<int>(transition_.nextScreen));
    std::unique_ptr<ScreenInterface> newScreen;
    newScreen = ScreenFactory::createScreen(transition_.nextScreen, logger_,
                                            displayManager_, pcMetrics_, this);

    if (newScreen) {
        currentScreen_ = std::move(newScreen);
        screenState_.activeScreen = transition_.nextScreen;
        currentScreen_->onEnter();
        // logger_.debug("[UIController] Screen activated");
    } else {
        logger_.error("[UIController] Failed to create screen");
        requestsScreenTransition(ScreenName::BOOT);  // Fallback
    }
}

void UIController::resetTransitionState() {
    transition_.nextScreen = ScreenName::UNSET;
    transition_.state = ScreenTransitionState::IDLE;
    transition_.isActive = false;
    transition_.startTime = 0;
    // logger_.debug("[UIController] Transition state reset");
}

void UIController::processTouchInput() {
    static unsigned long lastTouchTime = 0;
    constexpr unsigned long kDebounceMs = 200;

    unsigned long currentTime = millis();
    if (currentTime - lastTouchTime < kDebounceMs) {
        logger_.debug("[UIController] Touch ignored due to debounce");
        return;
    }

    LGFX* lcd = displayManager_->getDisplay();
    int32_t x = 0, y = 0;
    if (lcd->getTouch(&x, &y)) {
        // logger_.debugf("[UIController] Touch at (%d,%d)", x, y);
        if (currentScreen_) {
            currentScreen_->handleTouch(x, y);
        } else {
            logger_.warning("[UIController] No screen to handle touch");
        }
        lastTouchTime = currentTime;
    }
}