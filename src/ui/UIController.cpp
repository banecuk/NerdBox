#include "UIController.h"

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
      actionHandler_(new ActionHandler(this, logger_, displayDriver_)) {
    if (!displayDriver_) {
        throw std::invalid_argument("DisplayDriver pointer cannot be null");
    }
}

UIController::~UIController() {
    // Smart pointers will automatically clean up
}

void UIController::initialize() { setScreen(ScreenName::BOOT); }

bool UIController::setScreen(ScreenName screenName) {
    if (screenName == screenState_.activeScreen) {
        logger_.warning("Screen already active: %d", static_cast<int>(screenName));
        return false;
    }

    if (currentScreen_) {
        logger_.debug("Clearing current screen");
        currentScreen_->onExit();
        currentScreen_.reset();
        clearDisplay();
    }

    // Create new screen instances each time using new + unique_ptr
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
        case ScreenName::UNSET:
            logger_.error("Attempted to set UNSET screen");
            return false;
    }

    logger_.debugf("[Heap] Screen created: %d", ESP.getFreeHeap());
    
    if (currentScreen_) {
        currentScreen_->onEnter();
        screenState_.activeScreen = screenName;
    }
    return true;
}

void UIController::draw() {
    if (currentScreen_) {
        currentScreen_->draw();
    }
}

void UIController::handleTouchInput() {
    if (!currentScreen_ || !displayDriver_) {
        return;
    }

    LGFX* lcd = displayDriver_->getDisplay();
    if (!lcd) return;

    static unsigned long lastTouchTime = 0;
    constexpr unsigned long kDebounceMs = 50;

    int32_t x = 0, y = 0;
    if (lcd->getTouch(&x, &y)) {
        unsigned long currentTime = millis();
        if (currentTime - lastTouchTime > kDebounceMs) {
            lcd->fillRect(x - 2, y - 2, 4, 4, TFT_RED);
            currentScreen_->handleTouch(x, y);
            lastTouchTime = currentTime;
        }
    }
}

void UIController::clearDisplay() {
    if (displayDriver_ && displayDriver_->getDisplay()) {
        displayDriver_->getDisplay()->fillScreen(TFT_BLACK);
    }
}

DisplayDriver* UIController::getDisplayDriver() const {
    return displayDriver_;
}