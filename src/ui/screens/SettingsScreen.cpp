#include "SettingsScreen.h"

SettingsScreen::SettingsScreen(ILogger &logger, UIController *uiController)
    : logger_(logger),
      lcd_(uiController->getDisplayManager()->getDisplay()),
      uiController_(uiController),
      widgetManager_(logger, uiController->getDisplayManager()->getDisplay()) {
    createWidgets();
    logger_.debugf("SettingsScreen constructor. Free heap: %d", ESP.getFreeHeap());
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
    logger_.info("Entering SettingsScreen");
    logger_.debugf("Free heap: %d", ESP.getFreeHeap());

    // Initialize All Widgets
    widgetManager_.initializeWidgets();
}

void SettingsScreen::onExit() {
    logger_.info("Exiting SettingsScreen");
    widgetManager_.cleanupWidgets();
    logger_.debugf("[Heap] Post-cleanup: %d", ESP.getFreeHeap());
}

void SettingsScreen::draw() {
    if (!lcd_ || uiController_->isTransitioning() ||
        !uiController_->tryAcquireDisplayLock()) {
        return;
    }

    lcd_->startWrite();
    lcd_->setTextColor(TFT_GREEN, TFT_RED);
    lcd_->setTextSize(1);
    lcd_->setCursor(240, 120);
    lcd_->printf("Draw: %d", draw_count_);
    lcd_->endWrite();

    uiController_->releaseDisplayLock();

    widgetManager_.updateAndDrawWidgets();

    draw_count_++;

    if (draw_count_ > 1000) {
        draw_count_ = 0;
    }
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