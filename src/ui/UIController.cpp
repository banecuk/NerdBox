#include "UIController.h"

#include <esp_task_wdt.h>

#include "core/ActionHandler.h"
#include "screens/BootScreen.h"
#include "screens/MainScreen.h"
#include "screens/SettingsScreen.h"

UIController::UIController(ILogger& logger, DisplayDriver* displayDriver,
                           PcMetrics& pcMetrics, SystemState::ScreenState& screenState)
    : logger_(logger),
      displayDriver_(displayDriver),
      pcMetrics_(pcMetrics),
      screenState_(screenState),
      actionHandler_(new ActionHandler(this, logger_)) {
    if (!displayDriver_) {
        throw std::invalid_argument("DisplayDriver pointer cannot be null");
    }
}

UIController::~UIController() {
    // Smart pointers will automatically clean up
}

void UIController::initialize() { setScreen(ScreenName::BOOT); }

bool UIController::setScreen(ScreenName screenName) {
    uint32_t start = millis();

    // if (Config::Watchdog::kEnableOnBoot) {
    esp_task_wdt_reset();  // Reset before starting transition
    //}

    if (isChangingScreen_) {
        logger_.error("SCREEN RECURSION DETECTED!");
        return false;
    }

    if (screenName == ScreenName::UNSET) {
        logger_.error("Attempted transition to UNSET screen");
        return false;
    }

    if (screenName == screenState_.activeScreen) {
        logger_.debug("Screen already active");
        return false;
    }

    isChangingScreen_ = true;

    if (currentScreen_) {
        logger_.debug("Clearing current screen");
        currentScreen_->onExit();
        yield();
        currentScreen_.reset();
        yield();
        clearDisplay();
        yield();
    }

    logger_.debug("Creating new screen...");
    switch (screenName) {
        case ScreenName::BOOT:
            currentScreen_.reset(new BootScreen(logger_, displayDriver_->getDisplay()));
            break;
        case ScreenName::MAIN:
            currentScreen_.reset(new MainScreen(logger_, pcMetrics_, this));
            break;
        case ScreenName::SETTINGS:
            currentScreen_.reset(new SettingsScreen(logger_, this));
            break;
    }

    logger_.debugf("[Heap] Screen created: %d", ESP.getFreeHeap());

    yield();

    if (currentScreen_) {
        logger_.debug("Calling onEnter() for new screen");
        currentScreen_->onEnter();
        screenState_.activeScreen = screenName;
    }

    isChangingScreen_ = false;

    esp_task_wdt_reset();  // Reset after transition

    logger_.debugf("Screen transition took %ums", millis() - start);
    return true;
}

void UIController::draw() {
    if (currentScreen_) {
        currentScreen_->draw();
    }
}

void UIController::handleTouchInput() {
    if (!currentScreen_ || !displayDriver_ || isHandlingTouch_) {
        return;
    }

    isHandlingTouch_ = true;

    LGFX* lcd = displayDriver_->getDisplay();
    if (!lcd) {
        isHandlingTouch_ = false;
        return;
    }

    static unsigned long lastTouchTime = 0;
    constexpr unsigned long kDebounceMs = 50;

    int32_t x = 0, y = 0;
    if (lcd->getTouch(&x, &y)) {
        unsigned long currentTime = millis();
        if (currentTime - lastTouchTime > kDebounceMs) {
            logger_.debugf("UIController::handleTouchInput at (%d,%d)", x, y);
            currentScreen_->handleTouch(x, y);
            lastTouchTime = currentTime;
        }
    }

    isHandlingTouch_ = false;
}

void UIController::clearDisplay() {
    if (displayDriver_ && displayDriver_->getDisplay()) {
        displayDriver_->getDisplay()->fillScreen(TFT_BLACK);
    }
}

DisplayDriver* UIController::getDisplayDriver() const { return displayDriver_; }