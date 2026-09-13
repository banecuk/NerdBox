#pragma once

#include <LovyanGFX.hpp>

#include "config/AppSettings.h"
#include "config/LgfxConfig.h"
#include "utils/logging/LoggerInterface.h"
#include "utils/SettingsStore.h"

class DisplayManager {
 public:
    DisplayManager(LGFX& display, LoggerInterface& logger, const AppSettings& config);

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

    // Blocking backlight ramp used by UiController's screen-transition fade
    // (see docs-local/03-visual-ux.md V7(a)) — steps the raw PWM level from
    // wherever it currently sits down to 0 (rampDownForTransition) or back up
    // to the persisted effective brightness (rampUpForTransition) over
    // `durationMs`. Deliberately synchronous: the caller owns keeping this to
    // a short, one-off transition effect rather than anything per-frame.
    // Neither touches brightness_/NVS — only the raw hardware level.
    void rampDownForTransition(uint32_t durationMs);
    void rampUpForTransition(uint32_t durationMs);

 private:
    // Re-evaluates whether dimming should currently be applied and, if that
    // changed, pushes the new effective brightness to the display.
    void updateDimState();

    // Applies brightness_ (or brightness_ dimmed by kDimAtNightPercent, if
    // isCurrentlyDimmed_) to the physical display. Never persists.
    void applyEffectiveBrightness();

    // What applyEffectiveBrightness() would currently set — the ramp's
    // implicit start/end point.
    uint8_t effectiveBrightness() const;

    // Shared step loop for rampDownForTransition()/rampUpForTransition().
    void rampRawBrightnessTo(uint8_t target, uint32_t durationMs);

    LGFX& display_;
    LoggerInterface& logger_;
    const AppSettings& config_;
    SettingsStore settingsStore_;

    uint8_t brightness_;

    bool dimAtNightEnabled_;
    bool isNightWindowActive_ = false;
    bool isCurrentlyDimmed_ = false;
};