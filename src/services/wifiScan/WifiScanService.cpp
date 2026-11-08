#include "WifiScanService.h"

#include <esp_wifi.h>
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
    const int16_t started =
        WiFi.scanNetworks(/*async=*/true, /*show_hidden=*/config_.wifiScanShowHidden,
                          /*passive=*/false, /*max_ms_per_chan=*/config_.wifiScanMaxMsPerChannel);
    scanStartMs_ = millis();

    // Async scanNetworks() returns WIFI_SCAN_RUNNING on success (or when a
    // scan is already in flight, which we then simply wait for). Anything
    // else means esp_wifi_scan_start() refused, e.g. the radio is still busy
    // with an earlier scan. That used to be ignored: the state went to
    // SCANNING and the next poll() reported a bogus FAILED.
    if (started != WIFI_SCAN_RUNNING) {
        data.state = WifiScanData::State::FAILED;
        data.lastScanDurationMs = 0;
        logger_.warningf("WiFi scan: driver refused to start a scan (%d)", started);
        return false;
    }

    data.state = WifiScanData::State::SCANNING;
    return true;
}

bool WifiScanService::poll(WifiScanData& data) {
    const int16_t result = WiFi.scanComplete();
    const unsigned long elapsed = millis() - scanStartMs_;
    const WifiScanMath::ScanPoll verdict =
        WifiScanMath::classifyScanPoll(result, elapsed, config_.wifiScanResultTimeoutMs);
    if (verdict == WifiScanMath::ScanPoll::Running) {
        // Includes the core's premature WIFI_SCAN_FAILED (its own
        // max_ms_per_chan * 20 timeout): the radio is still scanning and
        // its SCAN_DONE event will turn scanComplete() into the count.
        return false;
    }

    data.lastScanDurationMs = static_cast<uint16_t>(std::min<unsigned long>(elapsed, 0xFFFFu));

    if (verdict == WifiScanMath::ScanPoll::Failed) {
        logger_.warningf("WiFi scan: no result after %lu ms, stopping it", elapsed);
        stopRadioScan();
        data.state = WifiScanData::State::FAILED;
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
    const WifiScanMath::ScanPoll verdict = WifiScanMath::classifyScanPoll(
        result, millis() - scanStartMs_, config_.wifiScanResultTimeoutMs);
    if (verdict == WifiScanMath::ScanPoll::Running) {
        // Still running off-screen (possibly past the core's own timeout);
        // checked again next tick. Marking it IDLE now would let the next
        // visit start a scan the radio rejects as busy.
        return;
    }
    if (verdict == WifiScanMath::ScanPoll::Failed) {
        stopRadioScan();
    } else {
        WiFi.scanDelete();
    }
    data.state = WifiScanData::State::IDLE;
}

void WifiScanService::stopRadioScan() {
    // Abort a scan the radio never finished, so the next esp_wifi_scan_start()
    // isn't refused as busy. Harmless if no scan is running. scanDelete()
    // frees any (late) result list the core may still hold.
    esp_wifi_scan_stop();
    WiFi.scanDelete();
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
