#pragma once

#include "services/wifiScan/WifiScanData.h"
#include "ui/widgets/base/Widget.h"

// Top-N nearby-SSID list for WifiScreen — direct structural copy of
// ProcessListWidget's row-diffed approach: onDraw formats each row into a
// stack buffer and repaints only rows whose text/colour-relevant fields
// changed. See docs-local/13-wifi-screen-plan.md §4.3.
class WifiScanListWidget : public Widget {
 public:
    WifiScanListWidget(const WidgetInterface::Dimensions& dims, uint32_t updateIntervalMs,
                       const WifiScanData& data);

 protected:
    void onDraw(bool forceRedraw) override;
    void onDrawStatic() override;

 private:
    static constexpr uint16_t kHeaderH = 16;
    static constexpr uint8_t kRows = 7;  // fits 149px (16 + 7*19); kMaxEntries (12) is the data cap
    static constexpr uint16_t kRowH = 19;

    static constexpr uint16_t kColBars = 6;
    static constexpr uint16_t kColSsid = 38;
    static constexpr uint16_t kColCh = 286;
    static constexpr uint16_t kColSec = 336;
    static constexpr uint16_t kColRssiRight = 472;  // right-aligned edge

    struct RowCache {
        char ssid[33] = "";
        uint8_t channel = 0;
        uint8_t auth = 0;
        int8_t rssi = 0;
        bool isCurrent = false;
        bool valid = false;
    };

    const WifiScanData& data_;
    WifiScanData::State lastState_ = WifiScanData::State::IDLE;
    uint8_t lastCount_ = 0;
    unsigned long lastFreshnessMs_ = 0;
    RowCache lastRows_[kRows];

    void drawHeader();
    void drawRow(uint8_t row, const WifiApEntry& entry, bool forceRedraw, uint8_t ourChannel);
    void clearRow(uint8_t row);
    void drawEmptyState(const char* message, uint16_t color);
};
