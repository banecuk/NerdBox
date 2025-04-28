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

// void MainScreen::initialize()
// {
//     // Initialization code if needed
// }

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

        // Remove the simulated touch from here
        // btn_.handleTouch(24, 24); // <-- REMOVE THIS
    }

    // Logger Messages (Draws if new messages exist)
    // Check if queue exists and has content before processing
    // if (logger_.hasScreenMessages()) { // Add a method like this to your logger
    //      std::queue<String> screenMessages = logger_.getScreenMessages(); // Careful:
    //      copies queue? Better: process directly or pass reference
    //      // Example: Limit messages per draw cycle
    //      int messagesDrawn = 0;
    //      int maxMessagesPerCycle = 3;
    //      while (!screenMessages.empty() && messagesDrawn < maxMessagesPerCycle)
    //      {
    //          String msg = screenMessages.front();
    //          // Simple scrolling log area example (needs improvement for real
    //          scrolling) lcd_->setCursor(0, 320 - 20); // Example fixed position for
    //          latest log lcd_->fillRect(0, 320 - 20, lcd_->width(), 10, TFT_BLACK); //
    //          Clear line lcd_->setTextColor(TFT_GREEN, TFT_BLACK); lcd_->println(msg);
    //          screenMessages.pop(); // Remove from the *local copy*
    //          logger_.popScreenMessage(); // Add method to remove from original queue in
    //          logger messagesDrawn++;
    //      }
    // }

    // -----------

    // if (hmData_.is_updated)
    // {
    //     logger_.debug("HM draw");

    //     this->lcd_->setTextColor(TFT_WHITE, TFT_BLACK);
    //     lcd_->setTextSize(2);
    //     lcd_->setCursor(360, 64);
    //     lcd_->printf("CPU: %i", hmData_.cpu_load);
    //     lcd_->setCursor(360, 64 + 20);
    //     lcd_->printf("GPU: %i", hmData_.gpu_load);
    //     lcd_->setTextSize(1);
    //     hmData_.is_updated = false;
    //     btn_.handleTouch(24, 24); // Simulates touch within button area
    // }

    // std::queue<String> screenMessages = logger_.getScreenMessages();
    // while (!screenMessages.empty())
    // {
    //     String msg = screenMessages.front();
    //     lcd_->setCursor(0, 320 - 60);
    //     lcd_->println(msg);
    //     screenMessages.pop();
    // }
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
    if (!screenManager_) return;

    switch (action) {
        case ActionType::RESET_DEVICE:
            screenManager_->resetDevice();
            break;
        case ActionType::CYCLE_BRIGHTNESS:
            screenManager_->cycleBrightness();
            break;
        // Handle other actions
        default:
            logger_.warning("Unknown action requested: %d", static_cast<int>(action));
    }
}