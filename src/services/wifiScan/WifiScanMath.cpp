#include "WifiScanMath.h"

#include <cstring>

namespace WifiScanMath {

void insertSorted(WifiApEntry (&out)[WifiScanData::kMaxEntries], uint8_t& count,
                  const WifiApEntry& candidate) {
    constexpr uint8_t kMax = WifiScanData::kMaxEntries;

    // De-duplicate on BSSID: a repeater/mesh node beacons the same SSID from
    // several radios (distinct BSSIDs, kept separately) but a genuine repeat
    // scan of the same radio keeps whichever reading is stronger.
    for (uint8_t i = 0; i < count; ++i) {
        if (memcmp(out[i].bssid, candidate.bssid, sizeof(candidate.bssid)) == 0) {
            if (candidate.rssi <= out[i].rssi) {
                return;  // existing reading is already the stronger one
            }
            // Remove the weaker existing entry, then fall through to insert
            // the stronger reading at its correctly-sorted position below.
            for (uint8_t j = i; static_cast<uint8_t>(j + 1) < count; ++j) {
                out[j] = out[j + 1];
            }
            --count;
            break;
        }
    }

    if (count < kMax) {
        uint8_t pos = count;
        while (pos > 0 && out[pos - 1].rssi < candidate.rssi) {
            out[pos] = out[pos - 1];
            --pos;
        }
        out[pos] = candidate;
        ++count;
        return;
    }

    // Full: array stays sorted descending, so the weakest kept entry is
    // always the last one. Only replace it if the candidate is stronger —
    // this is the top-N cut that keeps the *strongest* N, not the first N
    // seen.
    if (candidate.rssi > out[kMax - 1].rssi) {
        uint8_t pos = kMax - 1;
        while (pos > 0 && out[pos - 1].rssi < candidate.rssi) {
            out[pos] = out[pos - 1];
            --pos;
        }
        out[pos] = candidate;
    }
}

uint8_t rssiToQuality(int8_t rssi) {
    if (rssi >= -30) {
        return 100;
    }
    if (rssi <= -90) {
        return 0;
    }
    return static_cast<uint8_t>((static_cast<int16_t>(rssi) + 90) * 100 / 60);
}

SignalTier signalTier(int8_t rssi) {
    if (rssi > -65) {
        return SignalTier::kStrong;
    }
    if (rssi > -75) {
        return SignalTier::kWarn;
    }
    if (rssi > -85) {
        return SignalTier::kDegraded;
    }
    return SignalTier::kWeak;
}

uint8_t countOnChannel(const WifiApEntry* entries, uint8_t count, uint8_t channel) {
    uint8_t n = 0;
    for (uint8_t i = 0; i < count; ++i) {
        if (!entries[i].isCurrent && entries[i].channel == channel) {
            ++n;
        }
    }
    return n;
}

const char* authName(uint8_t auth) {
    switch (auth) {
        case 0:
            return "OPEN";
        case 1:
            return "WEP";
        case 2:
            return "WPA";
        case 3:
            return "WPA2";
        case 4:
            return "WPA2";  // WIFI_AUTH_WPA_WPA2_PSK — mixed WPA/WPA2
        case 6:
            return "WPA3";  // WIFI_AUTH_WPA3_PSK
        case 7:
            return "WPA2/3";  // WIFI_AUTH_WPA2_WPA3_PSK — mixed
        default:
            return "?";  // enterprise/WAPI/OWE/newer modes
    }
}

}  // namespace WifiScanMath
