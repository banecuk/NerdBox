#include "utils/SettingsStore.h"

#include "utils/LogMacros.h"

SettingsStore::SettingsStore(const char* nvsNamespace, LoggerInterface& logger)
    : namespace_(nvsNamespace), logger_(logger) {}

uint8_t SettingsStore::getU8(const char* key, uint8_t defaultValue) {
    if (!prefs_.begin(namespace_, /*readOnly=*/true)) {
        LOG_DEBUGF(logger_, "SettingsStore: NVS namespace '%s' not found, using default",
                   namespace_);
        return defaultValue;
    }
    uint8_t value = prefs_.getUChar(key, defaultValue);
    prefs_.end();
    return value;
}

void SettingsStore::putU8(const char* key, uint8_t value) {
    if (!prefs_.begin(namespace_, /*readOnly=*/false)) {
        logger_.errorf("SettingsStore: failed to open NVS namespace '%s' for writing", namespace_);
        return;
    }
    prefs_.putUChar(key, value);
    prefs_.end();
}

bool SettingsStore::getBool(const char* key, bool defaultValue) {
    if (!prefs_.begin(namespace_, /*readOnly=*/true)) {
        LOG_DEBUGF(logger_, "SettingsStore: NVS namespace '%s' not found, using default",
                   namespace_);
        return defaultValue;
    }
    bool value = prefs_.getBool(key, defaultValue);
    prefs_.end();
    return value;
}

void SettingsStore::putBool(const char* key, bool value) {
    if (!prefs_.begin(namespace_, /*readOnly=*/false)) {
        logger_.errorf("SettingsStore: failed to open NVS namespace '%s' for writing", namespace_);
        return;
    }
    prefs_.putBool(key, value);
    prefs_.end();
}
