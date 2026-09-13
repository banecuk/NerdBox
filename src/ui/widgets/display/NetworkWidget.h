#pragma once

#include "services/network/NetworkStatus.h"
#include "services/network/NetworkStatusService.h"
#include "ui/core/Colors.h"
#include "ui/widgets/base/Widget.h"

// Compact status bar widget — sits left of the clock in the bottom strip.
//
// Layout (148 × 24 px):
//
//   [ WiFi bars  68px ] [1px sep] [ Globe 36px ] [ 4×2 dots 40px ] [3px pad]
//
// WiFi section  : 4-bar signal-strength indicator.
// Globe section : internet reachability state coloured by severity.
// Dot grid      : 4 columns × 2 rows — one dot per probe endpoint. The dots
//                 are deliberately small so eight of them fit the same 40px
//                 section the previous six occupied — the widget's overall
//                 footprint is unchanged.
//                 White = endpoint OK, red = endpoint failed.
//
// A single failing endpoint does not colour the globe — see
// NetworkStatusService::recordResult(). Its dot still shows red.
//
// Internet state colours (globe), routed through Colors' semantic ramp
// instead of raw VGA-primitive hex (see docs-local/03-visual-ux.md V2):
//   OK       — light grey    (TFT_LIGHTGRAY)
//   WARNING  — muted gold    (Colors::kWarn)
//   DEGRADED — muted orange, the midpoint between kWarn and kDanger
//   DOWN     — muted red     (Colors::kDanger)
//   UNKNOWN  — dark grey     (Colors::kHairline)
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
    static constexpr uint16_t kDotSectionW = 40;    // 4×2 endpoint dots
    static constexpr uint16_t kPadRight = 3;

    // Bar geometry (WiFi signal bars)
    static constexpr uint8_t kBarCount = 4;
    static constexpr uint8_t kBarWidth = 6;
    static constexpr uint8_t kBarGap = 3;
    static constexpr uint8_t kBarBaseY = 2;  // bottom margin from widget bottom

    // Globe geometry
    static constexpr uint8_t kGlobeR = 9;  // radius px

    // Dot grid geometry
    static constexpr uint8_t kDotR = 2;  // radius of each dot
    static constexpr uint8_t kDotCols = 4;
    static constexpr uint8_t kDotRows = 2;
    static constexpr uint8_t kDotSpacX = 9;   // centre-to-centre horizontal
    static constexpr uint8_t kDotSpacY = 11;  // centre-to-centre vertical

    // The grid must stay inside its section — kDotSectionW never grows, the
    // dots shrink instead.
    static_assert((kDotCols - 1) * kDotSpacX + 2 * kDotR + 1 <= kDotSectionW,
                  "NetworkWidget's dot grid must fit within kDotSectionW");

    // This grid is a fixed 4x2 layout, one dot per probe endpoint — it does
    // not resize itself. If NetworkStatusService::kNumEndpoints ever changes,
    // this fails the build instead of silently drawing/reading past the grid.
    static_assert(static_cast<uint8_t>(kDotCols * kDotRows) == NetworkStatusService::kNumEndpoints,
                  "NetworkWidget's dot grid must have one cell per probe endpoint");

    // Colours
    static constexpr uint16_t kColorOk = TFT_LIGHTGRAY;
    static constexpr uint16_t kColorWarning = Colors::kWarn;
    static constexpr uint16_t kColorDegraded = 0xE3E8;  // muted orange, midpoint of kWarn/kDanger
    static constexpr uint16_t kColorDown = Colors::kDanger;
    static constexpr uint16_t kColorUnknown = Colors::kHairline;
    static constexpr uint16_t kColorDotFail = Colors::kDanger;  // failed-endpoint dot
    static constexpr uint16_t kColorDotOk = TFT_LIGHTGRAY;      // ok dot

    // -----------------------------------------------------------------------
    const NetworkStatus& status_;

    // Cached state for dirty detection
    bool lastConnected_ = false;
    int8_t lastRssiBracket_ = -1;
    NetworkStatus::Internet lastInternet_ = NetworkStatus::Internet::UNKNOWN;
    bool lastEndpointOk_[NetworkStatusService::kNumEndpoints] = {};
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
