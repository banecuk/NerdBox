#include "BaseWidgetScreen.h"

BaseWidgetScreen::BaseWidgetScreen(ILogger& logger, UIController* uiController)
    : logger_(logger),
      lcd_(uiController->getDisplayManager()->getDisplay()),
      uiController_(uiController),
      widgetManager_(logger, uiController->getDisplayManager()->getDisplay()) {}

BaseWidgetScreen::~BaseWidgetScreen() { logger_.debug("BaseWidgetScreen destructor"); }

void BaseWidgetScreen::onEnter() {
    createWidgets();
    widgetManager_.initializeWidgets();
}

void BaseWidgetScreen::onExit() { widgetManager_.cleanupWidgets(); }

void BaseWidgetScreen::draw() {
    if (!lcd_ || uiController_->isTransitioning() ||
        !uiController_->tryAcquireDisplayLock()) {
        return;
    }
    widgetManager_.updateAndDrawWidgets(false);
    uiController_->releaseDisplayLock();
}

void BaseWidgetScreen::handleTouch(uint16_t x, uint16_t y) {
    if (!lcd_) {
        logger_.debug("LCD not initialized, can't handle touch");
        return;
    }
    widgetManager_.handleTouch(x, y);
}

void BaseWidgetScreen::handleAction(EventType action) {
    EventBus::getInstance().publish(action);
}