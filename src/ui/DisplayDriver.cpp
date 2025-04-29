#include "DisplayDriver.h"

#include <Preferences.h>

DisplayDriver::DisplayDriver(LGFX& display, ILogger& logger)
    : display_(display), logger_(logger), brightness_(DEFAULT_BRIGHTNESS) {}

void DisplayDriver::initialize() {
    display_.init();
    display_.setRotation(1);  // Landscape
    display_.fillScreen(TFT_BLACK);
}

void DisplayDriver::postInitialization() { setBrightness(brightness_); }

LGFX* DisplayDriver::getDisplay() { return &display_; }

void DisplayDriver::setBrightness(uint8_t level) {
    brightness_ = level;
    display_.setBrightness(brightness_);
    logger_.infof("Brightness set to %d", brightness_);
}

uint8_t DisplayDriver::getBrightness() const { return brightness_; }

void DisplayDriver::cycleBrightness() {
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
