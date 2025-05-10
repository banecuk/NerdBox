#include "MainScreen.h"

MainScreen::MainScreen(ILogger &logger, PcMetrics &pcMetrics, UIController *uiController)
    : logger_(logger),
      lcd_(uiController->getDisplayManager()->getDisplay()),
      pcMetrics_(pcMetrics),
      uiController_(uiController),
      widgetManager_(logger, uiController->getDisplayManager()->getDisplay()) {
    createWidgets();
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
    widgetManager_.initializeWidgets();
}

void MainScreen::onExit() {
    widgetManager_.cleanupWidgets();
}

void MainScreen::draw() {
    if (!lcd_ || uiController_->isTransitioning() ||
        !uiController_->tryAcquireDisplayLock()) {
        return;
    }
    widgetManager_.updateAndDrawWidgets();
    uiController_->releaseDisplayLock();
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