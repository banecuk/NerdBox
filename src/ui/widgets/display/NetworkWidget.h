#pragma once

#include "services/network/NetworkStatus.h"
#include "ui/core/Colors.h"
#include "ui/widgets/base/Widget.h"

// Compact status bar widget — sits left of the clock in the bottom strip.
//
// Layout (148 × 24 px):
//
//   [ WiFi bars  68px ] [1px sep] [ Globe 36px ] [ 3×2 dots 40px ] [3px pad]
//
// WiFi section  : 4-bar signal-strength indicator.
// Globe section : internet reachability state coloured by severity.
// Dot grid      : 3 columns × 2 rows — one dot per probe endpoint.
//                 White = endpoint OK, red = endpoint failed.
//
// Internet state colours (globe):
//   OK       — white  (0xFFFF)
//   WARNING  — yellow (0xFFE0)
//   DEGRADED — orange (0xFC60)
//   DOWN     — red    (0xF800)
//   UNKNOWN  — dark grey (0x2104)
//
// Redraws only when RSSI bracket, wifi_connected, internet state, or any
// endpoint_ok flag changes.
class NetworkWidget : public Widget {
 public:
    NetworkWidget(const WidgetInterface::Dimensions& dims, uint32_t updateIntervalMs,
                  const NetworkStatus& status);

    bool handleTouch(uint16_t x, uint16_t y) override;

 protected:
    void onDraw(bool forceRedraw) override;
    void onDrawStatic() override;

 private:
    // -----------------------------------------------------------------------
    // Layout
    // -----------------------------------------------------------------------
    static constexpr uint16_t kWifiSectionW = 68;  // left — signal bars
    static constexpr uint16_t kSepW = 1;
    static constexpr uint16_t kGlobeSectionW = 36;  // globe icon
    static constexpr uint16_t kDotSectionW = 40;    // 3×2 endpoint dots
    static constexpr uint16_t kPadRight = 3;

    // Bar geometry (WiFi signal bars)
    static constexpr uint8_t kBarCount = 4;
    static constexpr uint8_t kBarWidth = 6;
    static constexpr uint8_t kBarGap = 3;
    static constexpr uint8_t kBarBaseY = 2;  // bottom margin from widget bottom

    // Globe geometry
    static constexpr uint8_t kGlobeR = 9;  // radius px

    // Dot grid geometry
    static constexpr uint8_t kDotR = 3;  // radius of each dot
    static constexpr uint8_t kDotCols = 3;
    static constexpr uint8_t kDotRows = 2;
    static constexpr uint8_t kDotSpacX = 12;  // centre-to-centre horizontal
    static constexpr uint8_t kDotSpacY = 11;  // centre-to-centre vertical

    // Colours
    static constexpr uint16_t kColorOk = TFT_LIGHTGRAY;
    static constexpr uint16_t kColorWarning = 0xFFE0;       // yellow
    static constexpr uint16_t kColorDegraded = 0xFC60;      // orange
    static constexpr uint16_t kColorDown = 0xF800;          // red
    static constexpr uint16_t kColorUnknown = Colors::kHairline;
    static constexpr uint16_t kColorDotFail = 0xF800;       // red dot
    static constexpr uint16_t kColorDotOk = TFT_LIGHTGRAY;  // ok dot

    // -----------------------------------------------------------------------
    const NetworkStatus& status_;

    // Cached state for dirty detection
    bool lastConnected_ = false;
    int8_t lastRssiBracket_ = -1;
    NetworkStatus::Internet lastInternet_ = NetworkStatus::Internet::UNKNOWN;
    bool lastEndpointOk_[6] = {false, false, false, false, false, false};
    bool lastInitialized_ = false;

    // -----------------------------------------------------------------------
    void drawWifi();
    void drawGlobe();
    void drawDotGrid();

    // Returns RGB565 color for the current wifi state
    uint16_t wifiColor() const;

    // Returns RGB565 color for the current internet state
    uint16_t internetColor() const;

    // RSSI bracket 0–4: number of filled wifi bars, also used for dirty detection
    int8_t rssiBracket() const;

    // True if any endpoint_ok flag differs from cached
    bool endpointsDirty() const;
};
