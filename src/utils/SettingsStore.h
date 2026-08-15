#pragma once

#include "utils/LoggerInterface.h"

#include <Preferences.h>

// Thin wrapper around NVS Preferences: collapses the
// begin()/get-or-default/end() (or begin()/put()/end()) boilerplate into one
// call each. Any subsystem persisting a runtime setting (brightness,
// dim-at-night, future refresh-rate/threshold settings) owns one of these
// instead of hand-rolling Preferences calls itself.
class SettingsStore {
 public:
    SettingsStore(const char* nvsNamespace, LoggerInterface& logger);

    uint8_t getU8(const char* key, uint8_t defaultValue);
    void putU8(const char* key, uint8_t value);

    bool getBool(const char* key, bool defaultValue);
    void putBool(const char* key, bool value);

 private:
    const char* namespace_;
    LoggerInterface& logger_;
    Preferences prefs_;
};
