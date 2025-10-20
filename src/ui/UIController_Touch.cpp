#include "UiController.h"

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