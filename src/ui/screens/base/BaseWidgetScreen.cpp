#include "BaseWidgetScreen.h"

#include "utils/logging/LogMacros.h"

BaseWidgetScreen::BaseWidgetScreen(LoggerInterface& logger, UiController* uiController,
                                   const AppSettings& config)
    : logger_(logger),
      config_(config),
      uiController_(uiController),
      widgetManager_(uiController->getDisplayContext(), uiController->getSystemMetrics()) {}

BaseWidgetScreen::~BaseWidgetScreen() {
    LOG_DEBUG(logger_, "BaseWidgetScreen destructor");
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
        LOG_DEBUG(logger_, "UIController not initialized, can't handle touch");
        return;
    }
    widgetManager_.handleTouch(x, y);
}

void BaseWidgetScreen::handleAction(EventType action) {
    EventBus::getInstance().publish(action);
}