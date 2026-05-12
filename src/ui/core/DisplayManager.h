#pragma once

#include <Preferences.h>
#include <LovyanGFX.hpp>

#include "config/AppConfig.h"
#include "config/LgfxConfig.h"
#include "utils/LoggerInterface.h"

class DisplayManager {
 public:
    DisplayManager(LGFX& display, LoggerInterface& logger);

    // Initialize the display hardware.
    void initialize();

    // Called after full system init: applies the persisted brightness level.
    void postInitialization();

    // Returns a pointer to the underlying display driver.
    LGFX* getDisplay();

    // Brightness control.
    // setBrightness() also persists the new level to NVS.
    void setBrightness(uint8_t level);
    uint8_t getBrightness() const;

    // Cycles through the fixed brightness steps and persists the result.
    void cycleBrightness();

 private:
    // Reads the saved brightness from NVS; returns kDefaultBrightness on miss.
    uint8_t loadBrightnessFromNvs();

    // Writes the current brightness_ to NVS.
    void saveBrightnessToNvs();

    LGFX& display_;
    LoggerInterface& logger_;
    Preferences prefs_;

    uint8_t brightness_;

    static constexpr uint8_t kDefaultBrightness =
        AppConfig::internal::UiImpl::kDefaultBrightness;
};