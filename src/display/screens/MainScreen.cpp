#include "MainScreen.h"

#include "display/widgets/ButtonCallbackImpl.h"

MainScreen::MainScreen(ILogger &logger, LGFX *lcd, PcMetrics &pcMetrics,
                       ScreenManager *screenManager)
    : logger_(logger),
      lcd_(lcd),
      pcMetrics_(pcMetrics),
      screenManager_(screenManager),
      widgetManager_(logger, lcd),
      button1_("Reset", {0, 0, 100, 40}, 0, ActionType::RESET_DEVICE,
               [this](ActionType action) { this->handleAction(action); }),
      button2_("Brightness", {110, 0, 100, 40}, 0, ActionType::CYCLE_BRIGHTNESS,
               [this](ActionType action) { this->handleAction(action); }),
      clockWidget_({330, 288, 150, 24}, 1000, TFT_LIGHTGREY, TFT_BLACK, 3) {
    // Add memory check
    logger_.debugf("MainScreen created. Free heap: %d", ESP.getFreeHeap());
}

// Destructor override
MainScreen::~MainScreen() {
    logger_.debug("MainScreen destructor");
    // Ensure cleanup happened in onExit. WidgetManager destructor will warn if not.
}

void MainScreen::onEnter() {
    logger_.info("Entering MainScreen");
    lcd_->setTextColor(TFT_WHITE, TFT_BLACK);
    lcd_->clear(TFT_BLACK);

    // Add debug info about buttons
    logger_.debugf("Button1 area: (%d,%d) to (%d,%d)", button1_.getDimensions().x,
                   button1_.getDimensions().y,
                   button1_.getDimensions().x + button1_.getDimensions().width,
                   button1_.getDimensions().y + button1_.getDimensions().height);

    logger_.debugf("Button2 area: (%d,%d) to (%d,%d)", button2_.getDimensions().x,
                   button2_.getDimensions().y,
                   button2_.getDimensions().x + button2_.getDimensions().width,
                   button2_.getDimensions().y + button2_.getDimensions().height);

    // --- Add Widgets to Manager ---
    widgetManager_.addWidget(&button1_);
    widgetManager_.addWidget(&button2_);
    widgetManager_.addWidget(&clockWidget_);

    // --- Initialize All Widgets ---
    widgetManager_.initializeWidgets();

    // --- Initial Draw ---
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

    // --- Update and Draw Widgets ---
    widgetManager_.updateAndDrawWidgets();

    // --- Draw Other Screen Elements (Non-Widget) ---

    // Hardware Monitor Data (Only redraw if updated)
    if (pcMetrics_.is_updated) {
        logger_.debug("HM data updated, drawing...");
        lcd_->setTextColor(TFT_WHITE, TFT_BLACK);
        lcd_->setTextSize(2);
        // Clear previous values before drawing new ones
        lcd_->fillRect(360, 64, 120, 40, TFT_BLACK);
        lcd_->setCursor(360, 64);
        lcd_->printf("CPU: %3d%%", pcMetrics_.cpu_load);  // Use % for percentage
        lcd_->setCursor(360, 64 + 20);
        lcd_->printf("GPU: %3d%%", pcMetrics_.gpu_load);  // Use %
        lcd_->setTextSize(1);

        pcMetrics_.is_updated = false;  // Reset update flag
    }
}

void MainScreen::handleTouch(uint16_t x, uint16_t y) {
    if (!lcd_) {
        logger_.debug("LCD not initialized, can't handle touch");
        return;
    }

    // logger_.debugf("MainScreen handling touch at (%d,%d)", x, y);
    bool handled = widgetManager_.handleTouch(x, y);

    if (!handled) {
        // logger_.debugf("No widget handled touch at (%d,%d)", x, y);
    }
}

void MainScreen::handleAction(ActionType action) {
    EventBus::getInstance().publish(action);
}