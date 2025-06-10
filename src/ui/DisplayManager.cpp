#include "DisplayManager.h"

//#include <Preferences.h>

DisplayManager::DisplayManager(LGFX& display, LoggerInterface& logger)
    : display_(display), logger_(logger), brightness_(DEFAULT_BRIGHTNESS) {}

void DisplayManager::initialize() {
    if (!display_.init()) {
        logger_.error("Display initialization failed");
        return;
    }
    display_.setRotation(1);  // Landscape
    display_.fillScreen(TFT_BLACK);
    display_.setBrightness(75);
}

void DisplayManager::postInitialization() { setBrightness(brightness_); }

LGFX* DisplayManager::getDisplay() { return &display_; }

void DisplayManager::setBrightness(uint8_t level) {
    brightness_ = level;
    display_.setBrightness(brightness_);
    logger_.infof("Brightness set to %d", brightness_);
}

uint8_t DisplayManager::getBrightness() const { return brightness_; }

void DisplayManager::cycleBrightness() {
    uint8_t brightness = 0;
    switch (getBrightness()) {
        case 255:
            brightness = 20;
            break;
        case 20:
            brightness = 75;
            break;
        case 75:
            brightness = 255;
            break;
        default:
            brightness = 75;
            break;
    }
    setBrightness(brightness);
}

