#include "BaseWidgetScreen.h"

BaseWidgetScreen::BaseWidgetScreen(LoggerInterface& logger, UiController* uiController,
                                   AppConfigInterface& config)
    : logger_(logger),
      uiController_(uiController),
      widgetManager_(uiController->getDisplayContext()),
      config_(config) {}

BaseWidgetScreen::~BaseWidgetScreen() {
    logger_.debug("BaseWidgetScreen destructor");
}

void BaseWidgetScreen::onEnter() {
    createWidgets();
    widgetManager_.initializeWidgets();
}

void BaseWidgetScreen::onExit() {
    widgetManager_.cleanupWidgets();
}

void BaseWidgetScreen::draw() {
    if (!uiController_ || uiController_->isTransitioning()) {
        return;
    }

    // Skip the semaphore entirely when nothing needs painting.
    // This runs lock-free on every idle frame (~60 fps); the mutex is only
    // taken when at least one widget is actually dirty or due for an update.
    if (!widgetManager_.hasAnyDirtyWidgets()) {
        return;
    }

    if (!uiController_->tryAcquireDisplayLock()) {
        return;
    }

    // Use optimized dirty widget updates instead of full redraw
    widgetManager_.updateDirtyWidgets();

    uiController_->releaseDisplayLock();
}

void BaseWidgetScreen::handleTouch(uint16_t x, uint16_t y) {
    if (!uiController_) {
        logger_.debug("UIController not initialized, can't handle touch");
        return;
    }
    widgetManager_.handleTouch(x, y);
}

void BaseWidgetScreen::handleAction(EventType action) {
    EventBus::getInstance().publish(action);
}