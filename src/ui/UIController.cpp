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
    drawMutex_ = xSemaphoreCreateMutex();
}

UIController::~UIController() {
}

void UIController::initialize() { setScreen(ScreenName::BOOT); }

bool UIController::setScreen(ScreenName screenName) {
    uint32_t start = millis();

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

    // Lock out all drawing
    drawLock_ = true;
    if (xSemaphoreTake(drawMutex_, portMAX_DELAY) != pdTRUE) {
        logger_.error("Failed to acquire draw mutex");
        return false;
    }

    isChangingScreen_ = true;
    displayDriver_->getDisplay()->startWrite(); // Start transaction

    if (currentScreen_) {
        logger_.debug("Clearing current screen");
        currentScreen_->onExit();
        currentScreen_.reset();
        clearDisplay();
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

    // logger_.debugf("[Heap] Screen created: %d", ESP.getFreeHeap());

    // Release locks
    xSemaphoreGive(drawMutex_);
    drawLock_ = false;

    if (currentScreen_) {
        logger_.debug("Calling onEnter() for new screen");
        screenState_.activeScreen = screenName;
        currentScreen_->onEnter();
    }

    displayDriver_->getDisplay()->endWrite(); // Start transaction

    isChangingScreen_ = false;

    logger_.debugf("Screen transition took %ums", millis() - start);
    return true;
}

void UIController::draw() {
    if ((currentScreen_)&&(!isChangingScreen_)) {
        currentScreen_->draw();
        handleTouchInput();
    }
}

void UIController::handleTouchInput() {
    if (isHandlingTouch_) {
        return;
    }

    static unsigned long lastTouchTime = 0;
    constexpr unsigned long kDebounceMs = 500;
    
    unsigned long currentTime = millis();
    if (currentTime - lastTouchTime > kDebounceMs) {
        isHandlingTouch_ = true;
        LGFX* lcd = displayDriver_->getDisplay();
        int32_t x = 0, y = 0;
        if (lcd->getTouch(&x, &y)) {
            logger_.debugf("UIController::handleTouchInput at (%d,%d)", x, y);
            currentScreen_->handleTouch(x, y);
            lastTouchTime = currentTime;
        }
        isHandlingTouch_ = false;
    } else {
        logger_.debug("Touch event ignored due to debounce");
    }

}

void UIController::clearDisplay() {
    if (displayDriver_ && displayDriver_->getDisplay()) {
        LGFX* lcd = displayDriver_->getDisplay();
        logger_.debug("Clear display");
        lcd->fillScreen(TFT_BLACK);      // Clear screen
        // lcd->waitDMA();                  // Wait for completion
    }
}

DisplayDriver* UIController::getDisplayDriver() const { return displayDriver_; }