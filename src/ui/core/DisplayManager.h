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

    // "Dim at night" — persists to NVS. Re-applies the effective brightness
    // immediately (dimmed or not, depending on whether it's currently night).
    void setDimAtNightEnabled(bool enabled);
    bool isDimAtNightEnabled() const;

    // Called by DimAtNightJob every background tick with whether the current
    // local time falls inside the night window. Cheap no-op unless the
    // night/day state actually changes.
    void setNightWindowActive(bool isNight);

 private:
    // Reads the saved brightness from NVS; returns kDefaultBrightness on miss.
    uint8_t loadBrightnessFromNvs();

    // Writes the current brightness_ to NVS.
    void saveBrightnessToNvs();

    bool loadDimAtNightFromNvs();
    void saveDimAtNightToNvs();

    // Re-evaluates whether dimming should currently be applied and, if that
    // changed, pushes the new effective brightness to the display.
    void updateDimState();

    // Applies brightness_ (or brightness_ dimmed by kDimAtNightPercent, if
    // isCurrentlyDimmed_) to the physical display. Never persists.
    void applyEffectiveBrightness();

    LGFX& display_;
    LoggerInterface& logger_;
    Preferences prefs_;

    uint8_t brightness_;

    bool dimAtNightEnabled_;
    bool isNightWindowActive_ = false;
    bool isCurrentlyDimmed_ = false;

    static constexpr uint8_t kDefaultBrightness =
        AppConfig::internal::UiImpl::kDefaultBrightness;
};