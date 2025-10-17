#include "UIController.h"

void UIController::processTouchInput() {
    static unsigned long lastTouchTime = 0;

    unsigned long currentTime = millis();
    if (currentTime - lastTouchTime < config_.getUiTouchDebounceIntervalMs()) {
        logger_.debug("[UIController] Touch ignored due to debounce");
        return;
    }

    LGFX* lcd = displayManager_->getDisplay();
    int32_t x = 0, y = 0;
    if (lcd->getTouch(&x, &y)) {
        if (currentScreen_) {
            currentScreen_->handleTouch(x, y);
        } else {
            logger_.warning("[UIController] No screen to handle touch");
        }
        lastTouchTime = currentTime;
    }
}