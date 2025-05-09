#include "MainScreen.h"

MainScreen::MainScreen(ILogger &logger, PcMetrics &pcMetrics, UIController *uiController)
    : logger_(logger),
      lcd_(uiController->getDisplayDriver()->getDisplay()),
      pcMetrics_(pcMetrics),
      uiController_(uiController),
      widgetManager_(logger, uiController->getDisplayDriver()->getDisplay()) {
    createWidgets();
    // logger_.debugf("MainScreen constructor. Free heap: %d", ESP.getFreeHeap());
}

MainScreen::~MainScreen() { logger_.debug("MainScreen destructor"); }

void MainScreen::createWidgets() {
    widgetManager_.addWidget(std::unique_ptr<ClockWidget>(
        new ClockWidget({328, 288, 150, 24}, 1000, TFT_LIGHTGREY, TFT_BLACK, 3)));
    widgetManager_.addWidget(std::unique_ptr<ButtonWidget>(
        new ButtonWidget("<", {0, 272, 48, 48}, 0, EventType::SHOW_SETTINGS,
                         [this](EventType action) { this->handleAction(action); })));
    widgetManager_.addWidget(std::unique_ptr<ButtonWidget>(
        new ButtonWidget("Brightness", {0, 0, 88, 48}, 0, EventType::CYCLE_BRIGHTNESS,
                         [this](EventType action) { this->handleAction(action); })));
}

void MainScreen::onEnter() {
    logger_.info("Entering MainScreen");

    // Initialize All Widgets
    widgetManager_.initializeWidgets();

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
    if (!lcd_ || uiController_->isTransitioning() ||
        !uiController_->tryAcquireDisplayLock()) {
        return;
    }

    if (draw_counter_ < 4) {
        logger_.debugf("MainScreen draw_counter_: %d", draw_counter_);
    }

    // static unsigned long lastHeapLog = 0;
    // if (millis() - lastHeapLog > 30000) {  // Every 30 seconds
    //     logger_.debugf("[Heap] Current free: %d", ESP.getFreeHeap());
    //     lastHeapLog = millis();
    // }

    lcd_->startWrite();
    lcd_->setTextColor(TFT_YELLOW, TFT_BLUE);
    lcd_->setTextSize(1);
    lcd_->setCursor(240, 160);
    lcd_->printf("Draw: %d", draw_counter_);
    lcd_->endWrite();

    uiController_->releaseDisplayLock();

    draw_counter_++;

    // Update and Draw Widgets
    widgetManager_.updateAndDrawWidgets();

    if (draw_counter_ > 1000) {
        draw_counter_ = 0;
    }
    if (draw_counter_ < 5) {
        logger_.debug("MainScreen draw completed");
    }

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

void MainScreen::handleAction(EventType action) {
    EventBus::getInstance().publish(action);
}