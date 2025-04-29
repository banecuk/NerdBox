#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>

#include <LovyanGFX.hpp>

#include "../config/LgfxConfig.h"
#include "Config.h"
#include "utils/ILogger.h"

class DisplayDriver {
   public:
    DisplayDriver(LGFX& display, ILogger& logger);

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
    void saveBrightnessToPreferences();

    void cycleBrightness();

   private:
    LGFX& display_;
    ILogger& logger_;

    uint8_t brightness_;

    static const uint8_t DEFAULT_BRIGHTNESS = 100;
};

extern DisplayDriver displayDriver;

#endif  // DISPLAY_MANAGER_H