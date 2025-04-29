#include "MainScreen.h"
#include "display/widgets/ButtonCallbackImpl.h"

MainScreen::MainScreen(ILogger &logger, LGFX *lcd, PcMetrics &pcMetrics,
                     ScreenManager *screenManager)
    : logger_(logger),
      lcd_(lcd),
      pcMetrics_(pcMetrics),
      screenManager_(screenManager),
      widgetManager_(logger, lcd),
      clockWidget_(new ClockWidget({330, 288, 150, 24}, 1000, TFT_LIGHTGREY, TFT_BLACK, 3)),
      button1_(new ButtonWidget("Reset", {0, 0, 100, 40}, 0, ActionType::RESET_DEVICE,
               [this](ActionType action) { this->handleAction(action); })),
      button2_(new ButtonWidget("Brightness", {110, 0, 100, 40}, 0, ActionType::CYCLE_BRIGHTNESS,
               [this](ActionType action) { this->handleAction(action); })) {
    logger_.debugf("MainScreen created. Free heap: %d", ESP.getFreeHeap());
}

MainScreen::~MainScreen() {
    logger_.debug("MainScreen destructor");
    // Widgets will be automatically cleaned up by unique_ptr
}

void MainScreen::onEnter() {
    logger_.info("Entering MainScreen");
    lcd_->setTextColor(TFT_WHITE, TFT_BLACK);
    lcd_->clear(TFT_BLACK);

    // Add debug info about buttons
    logger_.debugf("Button1 area: (%d,%d) to (%d,%d)", 
                  button1_->getDimensions().x,
                  button1_->getDimensions().y,
                  button1_->getDimensions().x + button1_->getDimensions().width,
                  button1_->getDimensions().y + button1_->getDimensions().height);

    logger_.debugf("Button2 area: (%d,%d) to (%d,%d)", 
                  button2_->getDimensions().x,
                  button2_->getDimensions().y,
                  button2_->getDimensions().x + button2_->getDimensions().width,
                  button2_->getDimensions().y + button2_->getDimensions().height);

    // Add Widgets to Manager (transfer ownership)
    widgetManager_.addWidget(std::unique_ptr<IWidget>(button1_.release()));
    widgetManager_.addWidget(std::unique_ptr<IWidget>(button2_.release()));
    widgetManager_.addWidget(std::unique_ptr<IWidget>(clockWidget_.release()));

    // Initialize All Widgets
    widgetManager_.initializeWidgets();

    // Initial Draw
    lcd_->setTextSize(2);
    lcd_->drawString("Main Dashboard", 5, 290);
    lcd_->setTextSize(1);

    widgetManager_.updateAndDrawWidgets(true);  // Force draw for all widgets on entry
}

void MainScreen::onExit() {
    logger_.info("Exiting MainScreen");
    widgetManager_.cleanupWidgets();
}

void MainScreen::draw() {
    if (!lcd_) return;  // Safety check

    // Update and Draw Widgets
    widgetManager_.updateAndDrawWidgets();

    // Draw Other Screen Elements (Non-Widget)
    if (pcMetrics_.is_updated) {
        logger_.debug("HM data updated, drawing...");
        lcd_->setTextColor(TFT_WHITE, TFT_BLACK);
        lcd_->setTextSize(2);
        lcd_->fillRect(360, 64, 120, 40, TFT_BLACK);
        lcd_->setCursor(360, 64);
        lcd_->printf("CPU: %3d%%", pcMetrics_.cpu_load);
        lcd_->setCursor(360, 64 + 20);
        lcd_->printf("GPU: %3d%%", pcMetrics_.gpu_load);
        lcd_->setTextSize(1);

        pcMetrics_.is_updated = false;
    }
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