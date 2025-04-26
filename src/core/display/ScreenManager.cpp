#include "core/display/ScreenManager.h"

ScreenManager::ScreenManager(ILogger &logger, LGFX *lcd, PcMetrics &pcMetrics,
                             SystemState::ScreenState &screenState)
    : logger_(logger),
      lcd_(lcd),
      pcMetrics_(pcMetrics),
      bootScreen_(logger, lcd),
      mainScreen_(nullptr),  // Initialize as nullptr
      screenState_(screenState) {
    if (!lcd) {
        throw std::invalid_argument("LGFX pointer cannot be null");
    }

    // Create MainScreen after construction is complete
    mainScreen_ = new MainScreen(logger, lcd, pcMetrics, this);
}

void ScreenManager::initialize() { setScreen(ScreenName::BOOT); }

bool ScreenManager::setScreen(ScreenName screenName) {
    if (screenName == screenState_.activeScreen) {
        logger_.warning("Screen already active: %d", static_cast<int>(screenName));
        return false;
    }

    if (currentScreen_) {
        currentScreen_->onExit();
        this->lcd_->fillRect(0, 0, 480, 320, TFT_BLACK);
    }

    switch (screenName) {
        case ScreenName::BOOT:
            currentScreen_ = &bootScreen_;
            break;
        case ScreenName::MAIN:
            if (mainScreen_) {
                currentScreen_ = mainScreen_;
            }
            break;
    }

    screenState_.activeScreen = screenName;

    currentScreen_->onEnter();

    return true;
}

void ScreenManager::draw() {
    if (currentScreen_) {
        currentScreen_->draw();
    }
}

void ScreenManager::handleTouchInput() {
    if (!currentScreen_) {
        return;
    }

    static unsigned long lastTouchTime = 0;
    constexpr unsigned long kDebounceMs = 50;  // ms debounce

    int32_t x = 0, y = 0;
    if (lcd_->getTouch(&x, &y)) {
        unsigned long currentTime = millis();
        if (currentTime - lastTouchTime > kDebounceMs) {
            lcd_->fillRect(x - 2, y - 2, 5, 5, TFT_RED);
            lcd_->setCursor(360, 20);
            lcd_->printf("Touch:(%03d,%03d)", x, y);

            currentScreen_->handleTouch(x, y);
            lastTouchTime = currentTime;
        }
    }
}

void ScreenManager::resetDevice() {
    logger_.info("ScreenManager::resetDevice() called");

    // lcd_->fillRect(0, 210, 320, 10, TFT_RED);
    // Implement actual reset logic
    ESP.restart();
}

void ScreenManager::cycleBrightness() {
    logger_.info("ScreenManager::cycleBrightness() called");

    static uint8_t brightnessLevel = 0;
    brightnessLevel = (brightnessLevel + 1) % 3;  // Cycle through 3 levels

    uint8_t brightness;
    switch (brightnessLevel) {
        case 0:
            brightness = 25;
            break;
        case 1:
            brightness = 100;
            break;
        case 2:
            brightness = 255;
            break;
    }

    lcd_->setBrightness(brightness);
}
