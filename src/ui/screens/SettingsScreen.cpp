#include "SettingsScreen.h"

SettingsScreen::SettingsScreen(ILogger &logger, UIController *uiController)
    : logger_(logger),
      lcd_(uiController->getDisplayDriver()->getDisplay()),
      uiController_(uiController),
      widgetManager_(logger, uiController->getDisplayDriver()->getDisplay()) {
    createWidgets();
    logger_.debugf("SettingsScreen constructor. Free heap: %d", ESP.getFreeHeap());
}

SettingsScreen::~SettingsScreen() {
    logger_.debug("SettingsScreen destructor");
}

void SettingsScreen::createWidgets() {
    widgetManager_.addWidget(
        std::unique_ptr<ButtonWidget>(new ButtonWidget(
            "<", {0, 320 - 1 - 48, 48, 48}, 0, ActionType::SHOW_MAIN,
            [this](ActionType action) { this->handleAction(action); }
        ))
    );

    widgetManager_.addWidget(
        std::unique_ptr<ButtonWidget>(new ButtonWidget(
            "Reset", {0, 0, 100, 48}, 0, ActionType::RESET_DEVICE,
            [this](ActionType action) { this->handleAction(action); }
        ))
    );
    
    widgetManager_.addWidget(
        std::unique_ptr<ButtonWidget>(new ButtonWidget(
            "Brightness", {0, 52, 100, 48}, 0, ActionType::CYCLE_BRIGHTNESS,
            [this](ActionType action) { this->handleAction(action); }
        ))
    );
}

void SettingsScreen::onEnter() {
    logger_.info("Entering SettingsScreen");

    lcd_->setTextColor(TFT_WHITE, TFT_BLACK);
    lcd_->clear(TFT_BLACK);
    lcd_->setTextSize(2);
    lcd_->setTextDatum(TL_DATUM);
    lcd_->drawString("Settings Dashboard", 5, 250);
    lcd_->setTextSize(1);

    logger_.debugf("Free heap: %d", ESP.getFreeHeap());

    logger_.debug("SettingsScreen - Initialize widgets.");

    // Initialize All Widgets
    widgetManager_.initializeWidgets();

    logger_.debug("SettingsScreen - Widgets initialized");
}

void SettingsScreen::onExit() {
    logger_.info("Exiting MainScreen");
    //widgetManager_.cleanupWidgets();
}

void SettingsScreen::draw() {
    if (!lcd_) return;  // Safety check

    // Update and Draw Widgets
    // logger_.info("Drawing SettingsScreen");
    widgetManager_.updateAndDrawWidgets();
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