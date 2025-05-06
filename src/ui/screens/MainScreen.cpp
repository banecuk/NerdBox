#include "MainScreen.h"

MainScreen::MainScreen(ILogger &logger, PcMetrics &pcMetrics, UIController *uiController)
    : logger_(logger),
      lcd_(uiController->getDisplayDriver()->getDisplay()),
      pcMetrics_(pcMetrics),
      uiController_(uiController),
      widgetManager_(logger, uiController->getDisplayDriver()->getDisplay()) {
    createWidgets();
    logger_.debugf("MainScreen constructor. Free heap: %d", ESP.getFreeHeap());
}

MainScreen::~MainScreen() { logger_.debug("MainScreen destructor"); }

void MainScreen::createWidgets() {
    widgetManager_.addWidget(std::unique_ptr<ClockWidget>(
        new ClockWidget({328, 288, 150, 24}, 1000, TFT_LIGHTGREY, TFT_BLACK, 3)));
    widgetManager_.addWidget(std::unique_ptr<ButtonWidget>(
        new ButtonWidget("<", {0, 272, 48, 48}, 0, ActionType::SHOW_SETTINGS,
                         [this](ActionType action) { this->handleAction(action); })));
    widgetManager_.addWidget(std::unique_ptr<ButtonWidget>(
        new ButtonWidget("Brightness", {0, 0, 88, 48}, 0, ActionType::CYCLE_BRIGHTNESS,
                         [this](ActionType action) { this->handleAction(action); })));
}

void MainScreen::onEnter() {
    logger_.info("Entering MainScreen");

    lcd_->fillRect(0, 0, 480, 320, TFT_NAVY);
    lcd_->drawSmoothLine(0, 160, 480, 80, TFT_BLACK);
    lcd_->drawSmoothLine(0, 160, 480, 240, TFT_BLACK);

    // Add debug info about buttons
    // logger_.debugf("Button1 area: (%d,%d) to (%d,%d)", button1_->getDimensions().x,
    //                button1_->getDimensions().y,
    //                button1_->getDimensions().x + button1_->getDimensions().width,
    //                button1_->getDimensions().y + button1_->getDimensions().height);

    // logger_.debugf("Button2 area: (%d,%d) to (%d,%d)", button2_->getDimensions().x,
    //                button2_->getDimensions().y,
    //                button2_->getDimensions().x + button2_->getDimensions().width,
    //                button2_->getDimensions().y + button2_->getDimensions().height);

    // Add Widgets to Manager (transfer ownership)

    // Initialize All Widgets
    widgetManager_.initializeWidgets();

    // Initial Draw
    // lcd_->setTextColor(TFT_WHITE, TFT_BLACK);
    // lcd_->clear(TFT_BLACK);
    // lcd_->setTextSize(2);
    // lcd_->setTextDatum(TL_DATUM);
    // lcd_->drawString("Main Dashboard", 5, 250);
    // lcd_->setTextSize(1);

    widgetManager_.updateAndDrawWidgets(true);  // Force draw for all widgets on entry
}

void MainScreen::onExit() {
    logger_.info("MainScreen::onExit() - START");
    logger_.debugf("[Heap] Pre-cleanup: %d", ESP.getFreeHeap());

    widgetManager_.cleanupWidgets();
    logger_.debugf("Widget count after cleanup: %d", widgetManager_.getWidgetCount());

    logger_.debugf("[Heap] Post-cleanup: %d", ESP.getFreeHeap());
    logger_.info("MainScreen::onExit() - COMPLETE");
}

void MainScreen::draw() {
    if (!lcd_) return;  // Safety check

    static unsigned long lastHeapLog = 0;
    if (millis() - lastHeapLog > 30000) {  // Every 30 seconds
        logger_.debugf("[Heap] Current free: %d", ESP.getFreeHeap());
        lastHeapLog = millis();
    }

    // Update and Draw Widgets
    widgetManager_.updateAndDrawWidgets();

    // Draw Non-Widget
    // if (pcMetrics_.is_updated) {
    //     logger_.debug("HM data drawing...");
    //     lcd_->setTextColor(TFT_WHITE, TFT_BLACK);
    //     lcd_->setTextSize(2);
    //     lcd_->fillRect(360, 64, 120, 40, TFT_BLACK);
    //     lcd_->setCursor(360, 64);
    //     lcd_->printf("CPU: %3d%%", pcMetrics_.cpu_load);
    //     lcd_->setCursor(360, 64 + 20);
    //     lcd_->printf("GPU: %3d%%", pcMetrics_.gpu_load);
    //     lcd_->setTextSize(1);

    //     pcMetrics_.is_updated = false;
    // }
}

void MainScreen::handleTouch(uint16_t x, uint16_t y) {
    if (!lcd_) {
        logger_.debug("LCD not initialized, can't handle touch");
        return;
    }
    widgetManager_.handleTouch(x, y);
}

void MainScreen::handleAction(ActionType action) {
    EventBus::getInstance().publish(action);
}