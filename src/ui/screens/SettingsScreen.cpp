#include "SettingsScreen.h"

SettingsScreen::SettingsScreen(ILogger &logger, UIController *uiController)
    : logger_(logger),
      lcd_(uiController->getDisplayDriver()->getDisplay()),
      uiController_(uiController),
      widgetManager_(logger, uiController->getDisplayDriver()->getDisplay()) {
    createWidgets();
    logger_.debugf("SettingsScreen constructor. Free heap: %d", ESP.getFreeHeap());
}

SettingsScreen::~SettingsScreen() { logger_.debug("SettingsScreen destructor"); }

void SettingsScreen::createWidgets() {
    widgetManager_.addWidget(std::unique_ptr<ClockWidget>(
        new ClockWidget({328, 288-24, 150, 24}, 1000, TFT_YELLOW, TFT_BLACK, 3)));

    widgetManager_.addWidget(std::unique_ptr<ButtonWidget>(
        new ButtonWidget("<", {52, 320 - 1 - 48, 48, 48}, 0, ActionType::SHOW_MAIN,
                         [this](ActionType action) { this->handleAction(action); })));

    // widgetManager_.addWidget(std::unique_ptr<ButtonWidget>(
    //     new ButtonWidget("Reset", {20, 20, 100, 48}, 0, ActionType::RESET_DEVICE,
    //                      [this](ActionType action) { this->handleAction(action); })));

    // widgetManager_.addWidget(std::unique_ptr<ButtonWidget>(
    //     new ButtonWidget("Brightness", {20, 72, 100, 48}, 0, ActionType::CYCLE_BRIGHTNESS,
    //                      [this](ActionType action) { this->handleAction(action); })));
}

void SettingsScreen::onEnter() {
    logger_.info("Entering SettingsScreen");

    // lcd_->setTextColor(TFT_WHITE, TFT_BLACK);
    // lcd_->clear(TFT_BLACK);
    // lcd_->setTextSize(2);
    // lcd_->setTextDatum(TL_DATUM);
    // lcd_->drawString("Settings Dashboard", 25, 250);
    // lcd_->setTextSize(1);

    logger_.debugf("Free heap: %d", ESP.getFreeHeap());

    logger_.debug("SettingsScreen - Initialize widgets.");

    // Initialize All Widgets
    widgetManager_.initializeWidgets();

    logger_.debug("SettingsScreen - Widgets initialized");
}

void SettingsScreen::onExit() {
    logger_.info("Exiting SettingsScreen");
    widgetManager_.cleanupWidgets();
    logger_.debugf("[Heap] Post-cleanup: %d", ESP.getFreeHeap());
}

void SettingsScreen::draw() {
    if (!lcd_ || uiController_->isChangingScreen() || !uiController_->tryAcquireDrawLock()) {
        return;
    }

    if (draw_counter_ < 5) {
        logger_.debugf("SettingsScreen draw_counter_: %d", draw_counter_);
    }    

    lcd_->setTextColor(TFT_GREEN, TFT_RED);
    lcd_->setTextSize(1);
    lcd_->setCursor(120, 120);
    lcd_->printf("Draw: %d", draw_counter_);

    uiController_->releaseDrawLock();    

    // Update and Draw Widgets
    // logger_.info("Drawing SettingsScreen");
    widgetManager_.updateAndDrawWidgets();

    draw_counter_++;

    if (draw_counter_ > 1000) {
        draw_counter_ = 0;
    }
    if (draw_counter_ < 6) {
        logger_.debug("SettingsScreen draw completed");
    }
    

}

void SettingsScreen::handleTouch(uint16_t x, uint16_t y) {
    if (!lcd_) {
        logger_.debug("LCD not initialized, can't handle touch");
        return;
    }
    widgetManager_.handleTouch(x, y);
}

void SettingsScreen::handleAction(ActionType action) {
    EventBus::getInstance().publish(action);
}