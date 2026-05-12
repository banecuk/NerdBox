#include "DisplayManager.h"

DisplayManager::DisplayManager(LGFX& display, LoggerInterface& logger)
    : display_(display), logger_(logger), brightness_(kDefaultBrightness) {}

void DisplayManager::initialize() {
    if (!display_.init()) {
        logger_.error("Display initialization failed");
        return;
    }
    display_.setRotation(1);  // Landscape
    display_.fillScreen(TFT_BLACK);

    // Use a safe low brightness during the init splash; postInitialization()
    // will switch to the user's saved level once the full system is up.
    display_.setBrightness(20);
}

void DisplayManager::postInitialization() {
    // Load whatever the user last chose from NVS, then apply it.
    brightness_ = loadBrightnessFromNvs();
    setBrightness(brightness_);
}

LGFX* DisplayManager::getDisplay() {
    return &display_;
}

void DisplayManager::setBrightness(uint8_t level) {
    brightness_ = level;
    display_.setBrightness(brightness_);
    logger_.infof("Brightness set to %d", brightness_);
    saveBrightnessToNvs();
}

uint8_t DisplayManager::getBrightness() const {
    return brightness_;
}

void DisplayManager::cycleBrightness() {
    uint8_t next;
    switch (brightness_) {
        case 20:  next = 75;  break;
        case 75:  next = 255; break;
        case 255: next = 20;  break;
        default:  next = 75;  break;
    }
    setBrightness(next);  // persists via setBrightness()
}

// ---------------------------------------------------------------------------
// Private NVS helpers
// ---------------------------------------------------------------------------

uint8_t DisplayManager::loadBrightnessFromNvs() {
    // open read-only; returns false if namespace doesn't exist yet — that's fine
    if (!prefs_.begin(AppConfig::internal::UiImpl::kNvsNamespace, /*readOnly=*/true)) {
        logger_.debug("DisplayManager: NVS namespace not found, using default brightness");
        return kDefaultBrightness;
    }

    uint8_t saved = prefs_.getUChar(AppConfig::internal::UiImpl::kNvsBrightnessKey,
                                     kDefaultBrightness);
    prefs_.end();

    logger_.infof("DisplayManager: loaded brightness %d from NVS", saved);
    return saved;
}

void DisplayManager::saveBrightnessToNvs() {
    if (!prefs_.begin(AppConfig::internal::UiImpl::kNvsNamespace, /*readOnly=*/false)) {
        logger_.error("DisplayManager: failed to open NVS for writing");
        return;
    }

    prefs_.putUChar(AppConfig::internal::UiImpl::kNvsBrightnessKey, brightness_);
    prefs_.end();

    logger_.debugf("DisplayManager: saved brightness %d to NVS", brightness_);
}
