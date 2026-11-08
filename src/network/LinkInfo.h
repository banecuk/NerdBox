#pragma once

#include <cstdint>

// This device's own WiFi link: SSID/BSSID/channel/security plus IP config.
// Filled by NetworkManager::fillLinkInfo() on the background task and
// published as a snapshot in NetworkStatus (see NetworkStatus::copyLink()),
// so widgets and web handlers never query the WiFi driver themselves (A8).
// All-empty/zero with connected == false when not connected. No heap.
struct LinkInfo {
    char ssid[33];
    char ip[16];
    char gateway[16];
    char mask[16];
    char dns[16];
    char mac[18];       // AA:BB:CC:DD:EE:FF
    char hostname[64];  // config_.networkMdnsHostname + ".local"
    uint8_t bssid[6];
    int8_t rssi;
    uint8_t channel;
    uint8_t auth;
    bool connected;
};
