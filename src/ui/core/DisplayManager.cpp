#include "DisplayManager.h"

#include "config/Limits.h"

DisplayManager::DisplayManager(LGFX& display, LoggerInterface& logger, const AppSettings& config)
    : display_(display),
      logger_(logger),
      config_(config),
      settingsStore_(config_.uiNvsNamespace, logger_),
      brightness_(config_.uiDefaultBrightness),
      dimAtNightEnabled_(config_.uiDefaultDimAtNightEnabled) {}

void DisplayManager::initialize() {
    if (!display_.init()) {
        logger_.error("Display initialization failed");
        return;
    }
    display_.setRotation(1);  // Landscape
    display_.fillScreen(TFT_BLACK);

    // LGFX's pushImage() fast path assumes raw uint16_t buffers are already
    // byte-swapped (swap565_t) unless told otherwise. Our PROGMEM icon arrays
    // (weather_icons_44.h, icon_gear) are plain rgb565_t bit-packed values, so
    // without this the R/B channels effectively scramble on push — a solid
    // orange sun icon renders as a noisy purple disc with a speckled edge.
    display_.setSwapBytes(true);

    // Use a safe low brightness during the init splash; postInitialization()
    // will switch to the user's saved level once the full system is up.
    display_.setBrightness(20);
}

void DisplayManager::postInitialization() {
    // Load whatever the user last chose from NVS, then apply it.
    brightness_ = settingsStore_.getU8(config_.uiNvsBrightnessKey, config_.uiDefaultBrightness);
    dimAtNightEnabled_ =
        settingsStore_.getBool(config_.uiNvsDimAtNightKey, config_.uiDefaultDimAtNightEnabled);
    applyEffectiveBrightness();
}

LGFX* DisplayManager::getDisplay() {
    return &display_;
}

void DisplayManager::setBrightness(uint8_t level) {
    brightness_ = level;
    logger_.infof("Brightness set to %d", brightness_);
    settingsStore_.putU8(config_.uiNvsBrightnessKey, brightness_);
    applyEffectiveBrightness();
}

uint8_t DisplayManager::getBrightness() const {
    return brightness_;
}

void DisplayManager::cycleBrightness() {
    const uint8_t* levels = config_.uiBrightnessLevels;
    const uint8_t count = AppConfig::Limits::kBrightnessLevelCount;

    // Find the current level in the array, advance to next (wrapping).
    uint8_t nextIndex = 0;
    for (uint8_t i = 0; i < count; ++i) {
        if (levels[i] == brightness_) {
            nextIndex = (i + 1) % count;
            break;
        }
    }
    setBrightness(levels[nextIndex]);
}

void DisplayManager::setDimAtNightEnabled(bool enabled) {
    dimAtNightEnabled_ = enabled;
    logger_.infof("Dim at night %s", enabled ? "enabled" : "disabled");
    settingsStore_.putBool(config_.uiNvsDimAtNightKey, dimAtNightEnabled_);
    updateDimState();
}

bool DisplayManager::isDimAtNightEnabled() const {
    return dimAtNightEnabled_;
}

void DisplayManager::setNightWindowActive(bool isNight) {
    if (isNight == isNightWindowActive_)
        return;
    isNightWindowActive_ = isNight;
    updateDimState();
}

void DisplayManager::updateDimState() {
    const bool shouldDim = dimAtNightEnabled_ && isNightWindowActive_;
    if (shouldDim == isCurrentlyDimmed_)
        return;
    isCurrentlyDimmed_ = shouldDim;
    logger_.infof("Night dim %s (brightness %d)", isCurrentlyDimmed_ ? "applied" : "cleared",
                  brightness_);
    applyEffectiveBrightness();
}

void DisplayManager::applyEffectiveBrightness() {
    uint8_t effective = brightness_;
    if (isCurrentlyDimmed_) {
        effective = static_cast<uint8_t>(
            (static_cast<uint16_t>(brightness_) * (100 - config_.uiDimAtNightPercent)) / 100);
    }
    display_.setBrightness(effective);
}
