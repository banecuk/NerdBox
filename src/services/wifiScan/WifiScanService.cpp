#include "WifiScanService.h"

#include <WiFi.h>

#include <algorithm>
#include <cstring>

#include "services/wifiScan/WifiScanMath.h"

WifiScanService::WifiScanService(LoggerInterface& logger, const AppSettings& config)
    : logger_(logger), config_(config) {}

bool WifiScanService::start(WifiScanData& data) {
    if (data.state == WifiScanData::State::SCANNING) {
        return false;
    }

    WiFi.scanDelete();  // in case a previous result was never drained
    WiFi.scanNetworks(/*async=*/true, /*show_hidden=*/config_.wifiScanShowHidden,
                      /*passive=*/false, /*max_ms_per_chan=*/config_.wifiScanMaxMsPerChannel);

    data.state = WifiScanData::State::SCANNING;
    scanStartMs_ = millis();
    return true;
}

bool WifiScanService::poll(WifiScanData& data) {
    const int16_t result = WiFi.scanComplete();
    if (result == WIFI_SCAN_RUNNING) {
        return false;
    }

    data.lastScanDurationMs =
        static_cast<uint16_t>(std::min<unsigned long>(millis() - scanStartMs_, 0xFFFFu));

    if (result < 0) {  // WIFI_SCAN_FAILED (or any other negative surprise)
        data.state = WifiScanData::State::FAILED;
        WiFi.scanDelete();
        return true;
    }

    applyResults(data, result);
    WiFi.scanDelete();  // driver holds the result list until deleted — must
                        // always be called on every completed scan (§3.4)
    data.state = WifiScanData::State::DONE;
    data.freshness.publish(millis());
    return true;
}

void WifiScanService::abandon(WifiScanData& data) {
    const int16_t result = WiFi.scanComplete();
    if (result == WIFI_SCAN_RUNNING) {
        return;  // still running off-screen; checked again next tick
    }
    if (result >= 0) {
        WiFi.scanDelete();
    }
    data.state = WifiScanData::State::IDLE;
}

void WifiScanService::applyResults(WifiScanData& data, int16_t resultCount) {
    data.count = 0;
    data.totalFound = static_cast<uint8_t>(std::min<int16_t>(resultCount, 255));

    uint8_t ourBssid[6] = {0};
    bool haveOurBssid = false;
    if (WiFi.status() == WL_CONNECTED) {
        const uint8_t* b = WiFi.BSSID();
        if (b) {
            memcpy(ourBssid, b, sizeof(ourBssid));
            haveOurBssid = true;
        }
    }

    for (int16_t i = 0; i < resultCount; ++i) {
        WifiApEntry entry;

        // WiFi.SSID(i) returns an Arduino String — copied into the fixed
        // buffer and dropped within this statement; nothing downstream ever
        // sees a String (see CLAUDE.md "Memory discipline").
        strlcpy(entry.ssid, WiFi.SSID(i).c_str(), sizeof(entry.ssid));

        entry.rssi = static_cast<int8_t>(WiFi.RSSI(i));
        entry.channel = static_cast<uint8_t>(WiFi.channel(i));
        entry.auth = static_cast<uint8_t>(WiFi.encryptionType(i));

        const uint8_t* bssid = WiFi.BSSID(i);
        if (bssid) {
            memcpy(entry.bssid, bssid, sizeof(entry.bssid));
        }
        entry.isCurrent = haveOurBssid && memcmp(entry.bssid, ourBssid, sizeof(ourBssid)) == 0;

        WifiScanMath::insertSorted(data.entries, data.count, entry);
    }

    const uint8_t ourChannel = static_cast<uint8_t>(WiFi.channel());
    data.sameChannelCount = WifiScanMath::countOnChannel(data.entries, data.count, ourChannel);
}
