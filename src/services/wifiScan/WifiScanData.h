#pragma once

#include <atomic>
#include <cstdint>

#include "config/Limits.h"
#include "utils/PublishedFlag.h"

// One scanned access point. Fixed-size only — the Arduino WiFi scan API hands
// back an Arduino String per SSID, which WifiScanService copies into `ssid`
// and releases immediately via WiFi.scanDelete(); no String ever reaches this
// struct or the widgets (CLAUDE.md "Memory discipline").
struct WifiApEntry {
    char ssid[33] = "";    // 32 chars + NUL; "" == hidden network
    uint8_t bssid[6] = {};  // used for de-duplication and "is this us?"
    int8_t rssi = 0;        // dBm
    uint8_t channel = 0;    // 1..14 — ESP32-S3 is 2.4 GHz only, so no band column
    uint8_t auth = 0;       // wifi_auth_mode_t, stored as uint8_t to keep esp_wifi.h out of ui/
    bool isCurrent = false;  // BSSID matches the AP we are associated with
};

// Scan result for the WIFI screen's "nearby networks" list — see
// docs-local/13-wifi-screen-plan.md. Written by WifiScanJob/WifiScanService on
// the background task, read by WifiScanListWidget on the screen task. No
// mutex: fixed arrays and scalars only, same convention as ProcessData.
struct WifiScanData {
    static constexpr uint8_t kMaxEntries = AppConfig::Limits::kWifiScanEntries;

    enum class State : uint8_t { IDLE, SCANNING, DONE, FAILED };

    WifiApEntry entries[kMaxEntries];  // sorted by RSSI, strongest first
    uint8_t count = 0;                 // entries actually populated (<= kMaxEntries)
    uint8_t totalFound = 0;            // APs the radio saw, before the top-N cut
    uint8_t sameChannelCount = 0;      // neighbours (excluding us) on our own channel

    State state = State::IDLE;
    uint16_t lastScanDurationMs = 0;
    std::atomic<bool> rescanRequested{false};  // set on screen entry / Rescan button

    PublishedFlag freshness;  // publish() on every completed scan
};
