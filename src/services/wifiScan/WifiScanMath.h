#pragma once

#include <cstdint>

#include "WifiScanData.h"

// Pure functions backing the WIFI screen's scan-result processing — no
// Arduino/esp_wifi.h dependency, host-tested under [env:native] (see
// test/WifiScanMathTest.cpp). Split out of WifiScanService so the top-N
// sort/de-dup/quality/channel-congestion logic can be tested without a radio.
namespace WifiScanMath {

// Insert into a fixed top-N array kept sorted by RSSI descending. De-duplicates
// on BSSID (repeaters/mesh nodes beacon the same SSID from several radios —
// distinct BSSIDs are kept separately; a genuine duplicate BSSID keeps the
// stronger reading).
void insertSorted(WifiApEntry (&out)[WifiScanData::kMaxEntries], uint8_t& count,
                  const WifiApEntry& candidate);

// -30 dBm -> 100%, -90 dBm -> 0%, clamped and linear in between. Matches the
// bracket boundaries NetworkWidget/WifiLinkWidget use for their signal bars.
uint8_t rssiToQuality(int8_t rssi);

// Signal-strength colour tier, shared by NetworkWidget, WifiLinkWidget and
// WifiScanListWidget so all three read an RSSI the same way instead of each
// hand-copying the same four thresholds (see docs-local/13-wifi-screen-plan.md
// §4.2 — precisely the C2 copy-paste failure mode). Boundaries: > -65 dBm
// strong, > -75 warn, > -85 degraded, else weak. The tier->colour mapping
// itself stays in ui/ (this header has no LGFX/Colors dependency, so it
// compiles under [env:native]).
enum class SignalTier : uint8_t {
    kStrong,
    kWarn,
    kDegraded,
    kWeak
};
SignalTier signalTier(int8_t rssi);

// Neighbours (excluding the entry marked isCurrent) sharing `channel`.
uint8_t countOnChannel(const WifiApEntry* entries, uint8_t count, uint8_t channel);

// wifi_auth_mode_t (passed as a plain uint8_t so this stays Arduino/ESP-IDF
// free) -> a short display label. Values follow ESP-IDF's wifi_auth_mode_t
// numbering (0=OPEN, 1=WEP, 2=WPA_PSK, 3=WPA2_PSK, 4=WPA_WPA2_PSK,
// 6=WPA3_PSK, 7=WPA2_WPA3_PSK); anything else (enterprise/WAPI/OWE/newer
// modes) falls back to "?".
const char* authName(uint8_t auth);

// Verdict on one WiFi.scanComplete() poll of an async scan.
enum class ScanPoll : uint8_t {
    Running,
    Done,
    Failed
};

// scanCompleteResult is WiFi.scanComplete()'s return: >= 0 is the result
// count, -1 (WIFI_SCAN_RUNNING) still running, -2 (WIFI_SCAN_FAILED) failed.
//
// The Arduino core's -2 can't be taken at face value. scanNetworks() arms its
// own timeout of max_ms_per_chan * 20 and scanComplete() reports -2 once it
// passes, while the radio is in fact still scanning. A scan while connected
// keeps returning to the home channel and regularly overruns that. When the
// radio's SCAN_DONE event does arrive, the core resets its timer and
// scanComplete() returns the count. So -1 and -2 both mean "keep waiting"
// until our own deadlineMs (counted from the start call); only then is it a
// failure. Pure logic; host-tested in test/WifiScanMathTest.cpp.
ScanPoll classifyScanPoll(int16_t scanCompleteResult, uint32_t elapsedMs, uint32_t deadlineMs);

}  // namespace WifiScanMath
