#include "SettingsScreen.h"

SettingsScreen::SettingsScreen(ILogger &logger, UIController *uiController)
    : logger_(logger),
      lcd_(uiController->getDisplayManager()->getDisplay()),
      uiController_(uiController),
      widgetManager_(logger, uiController->getDisplayManager()->getDisplay()) {
    createWidgets();
}

SettingsScreen::~SettingsScreen() { logger_.debug("SettingsScreen destructor"); }

void SettingsScreen::createWidgets() {
    widgetManager_.addWidget(std::unique_ptr<ClockWidget>(
        new ClockWidget({328, 288 - 24, 150, 24}, 1000, TFT_YELLOW, TFT_BLACK, 3)));

    widgetManager_.addWidget(std::unique_ptr<ButtonWidget>(
        new ButtonWidget("<", {0, 320 - 1 - 48, 48, 48}, 0, EventType::SHOW_MAIN,
                         [this](EventType action) { this->handleAction(action); })));

    widgetManager_.addWidget(std::unique_ptr<ButtonWidget>(
        new ButtonWidget("Reset", {20, 20, 100, 48}, 0, EventType::RESET_DEVICE,
                         [this](EventType action) { this->handleAction(action); })));

    widgetManager_.addWidget(std::unique_ptr<ButtonWidget>(
        new ButtonWidget("Brightness", {20, 72, 100, 48}, 0, EventType::CYCLE_BRIGHTNESS,
                         [this](EventType action) { this->handleAction(action); })));
}

void SettingsScreen::onEnter() {
    widgetManager_.initializeWidgets();
}

void SettingsScreen::onExit() {
    widgetManager_.cleanupWidgets();
}

void SettingsScreen::draw() {
    if (!lcd_ || uiController_->isTransitioning() ||
        !uiController_->tryAcquireDisplayLock()) {
        return;
    }
    widgetManager_.updateAndDrawWidgets();
    uiController_->releaseDisplayLock();
}

void SettingsScreen::handleTouch(uint16_t x, uint16_t y) {
    if (!lcd_) {
        logger_.debug("LCD not initialized, can't handle touch");
        return;
    }
    widgetManager_.handleTouch(x, y);
}

void SettingsScreen::handleAction(EventType action) {
    EventBus::getInstance().publish(action);
}