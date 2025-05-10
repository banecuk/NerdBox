#include "DisplayManager.h"

#include <Preferences.h>

DisplayManager::DisplayManager(LGFX& display, ILogger& logger)
    : display_(display), logger_(logger), brightness_(DEFAULT_BRIGHTNESS) {}

void DisplayManager::initialize() {
    if (!display_.init()) {
        logger_.error("Display initialization failed");
        return;
    }
    display_.setRotation(1);  // Landscape
    display_.fillScreen(TFT_BLACK);
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
            brightness = 25;
            break;
        case 25:
            brightness = 100;
            break;
        case 100:
            brightness = 255;
            break;
        default:
            brightness = 100;
            break;
    }
    setBrightness(brightness);
}

