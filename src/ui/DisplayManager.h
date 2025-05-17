#pragma once

#include <Arduino.h>

#include <LovyanGFX.hpp>

#include "../config/LgfxConfig.h"
#include "Config.h"
#include "utils/LoggerInterface.h"

class DisplayManager {
   public:
    DisplayManager(LGFX& display, LoggerInterface& logger);

    // Initialize the display
    void initialize();

    // Post-initialization tasks
    void postInitialization();

    // Get the display instance
    LGFX* getDisplay();

    // Brightness control methods
    void setBrightness(uint8_t level);
    uint8_t getBrightness() const;

    // Helper for saving brightness to preferences
    void saveBrightnessToPreferences();  // TODO: Implement this method

    void cycleBrightness();

   private:
    LGFX& display_;
    LoggerInterface& logger_;

    uint8_t brightness_;

    static const uint8_t DEFAULT_BRIGHTNESS = 100;
};