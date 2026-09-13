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
    // (weather_icons_44.h, icons_24.h) are plain rgb565_t bit-packed values, so
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
    display_.setBrightness(effectiveBrightness());
}

uint8_t DisplayManager::effectiveBrightness() const {
    if (!isCurrentlyDimmed_) {
        return brightness_;
    }
    return static_cast<uint8_t>(
        (static_cast<uint16_t>(brightness_) * (100 - config_.uiDimAtNightPercent)) / 100);
}

void DisplayManager::rampDownForTransition(uint32_t durationMs) {
    rampRawBrightnessTo(0, durationMs);
}

void DisplayManager::rampUpForTransition(uint32_t durationMs) {
    rampRawBrightnessTo(effectiveBrightness(), durationMs);
}

void DisplayManager::rampRawBrightnessTo(uint8_t target, uint32_t durationMs) {
    // ~10ms per step: fine-grained enough to look smooth over an 80-120ms
    // ramp, coarse enough that a handful of setBrightness() calls (a single
    // LEDC duty-cycle write each) doesn't itself eat into the budget.
    constexpr uint32_t kStepMs = 10;
    const uint8_t start = display_.getBrightness();
    const uint32_t steps = durationMs / kStepMs;

    if (start == target || steps == 0) {
        display_.setBrightness(target);
        return;
    }

    const int32_t delta = static_cast<int32_t>(target) - static_cast<int32_t>(start);
    for (uint32_t step = 1; step <= steps; ++step) {
        const int32_t level =
            static_cast<int32_t>(start) + delta * static_cast<int32_t>(step) / static_cast<int32_t>(steps);
        display_.setBrightness(static_cast<uint8_t>(level));
        delay(kStepMs);
    }
}
