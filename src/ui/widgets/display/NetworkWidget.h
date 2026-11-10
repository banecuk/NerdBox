#pragma once

#include <functional>

#include "core/events/EventTypes.h"
#include "services/network/NetworkStatus.h"
#include "services/network/NetworkStatusService.h"
#include "ui/core/Colors.h"
#include "ui/widgets/base/TapAction.h"
#include "ui/widgets/base/Widget.h"

// Compact status bar widget — sits left of the clock in the bottom strip.
//
// Layout (148 × 24 px):
//
//   [ WiFi bars  68px ] [1px sep] [ 4×2 dots 76px ] [3px pad]
//
// WiFi section  : 4-bar signal-strength indicator.
// Dot grid      : 4 columns × 2 rows — one dot per probe endpoint, centred
//                 in the space the old internet-state globe used to share
//                 (the globe was dropped: the dots already show it). The
//                 widget's overall footprint is unchanged.
//                 White = endpoint OK, red = endpoint failed, dark grey =
//                 not probed yet (e.g. right after boot).
//
// Redraws only when RSSI bracket, wifi_connected, or any endpoint_ok/
// endpoint_probed flag changes.
class NetworkWidget : public Widget {
 public:
    // Optional tap action (mirrors ClockWidget/AirQualityWidget): when a
    // callback is set, a tap publishes `action`, e.g. to open the WIFI
    // screen. Defaults keep every other NetworkWidget instance (this
    // screen's own untappable copy on WifiScreen) non-tappable. `dims` may be
    // taller than the widget's actual drawn content (kContentH) — see
    // contentTop()'s comment — so MainScreen can hand this a full band-height
    // tap target without changing what's painted (docs-local/
    // 13-wifi-screen-plan.md §4.5).
    NetworkWidget(const WidgetInterface::Dimensions& dims, uint32_t updateIntervalMs,
                  const NetworkStatus& status, TapAction tap = {});

    bool handleTouch(uint16_t x, uint16_t y) override;

 protected:
    void onDraw(bool forceRedraw) override;
    void onDrawStatic() override;

 private:
    // -----------------------------------------------------------------------
    // Layout
    // -----------------------------------------------------------------------
    // Fixed drawn-content height — every drawing routine below sizes/
    // positions itself against this and contentTop(), not dimensions_.height/
    // dimensions_.y, so a taller dimensions_ (a bigger tap target) grows only
    // the hit box: the content stays vertically centred at the same absolute
    // position it always was (see the constructor comment above).
    static constexpr uint16_t kContentH = 24;

    static constexpr uint16_t kWifiSectionW = 68;  // left — signal bars
    static constexpr uint16_t kSepW = 1;
    static constexpr uint16_t kDotSectionW = 76;  // 4×2 endpoint dots
    static constexpr uint16_t kPadRight = 3;

    // Bar geometry (WiFi signal bars)
    static constexpr uint8_t kBarCount = 4;
    static constexpr uint8_t kBarWidth = 6;
    static constexpr uint8_t kBarGap = 3;
    static constexpr uint8_t kBarBaseY = 2;  // bottom margin from widget bottom

    // Dot grid geometry
    static constexpr uint8_t kDotR = 2;  // radius of each dot
    static constexpr uint8_t kDotCols = 4;
    static constexpr uint8_t kDotRows = 2;
    static constexpr uint8_t kDotSpacX = 9;   // centre-to-centre horizontal
    static constexpr uint8_t kDotSpacY = 11;  // centre-to-centre vertical

    // The grid must stay inside its section.
    static_assert((kDotCols - 1) * kDotSpacX + 2 * kDotR + 1 <= kDotSectionW,
                  "NetworkWidget's dot grid must fit within kDotSectionW");

    // This grid is a fixed 4x2 layout, one dot per probe endpoint — it does
    // not resize itself. If NetworkStatusService::kNumEndpoints ever changes,
    // this fails the build instead of silently drawing/reading past the grid.
    static_assert(static_cast<uint8_t>(kDotCols * kDotRows) == NetworkStatusService::kNumEndpoints,
                  "NetworkWidget's dot grid must have one cell per probe endpoint");

    // Colours
    static constexpr uint16_t kColorWarning = Colors::kWarn;
    static constexpr uint16_t kColorDegraded = Colors::kDegraded;
    static constexpr uint16_t kColorDown = Colors::kDanger;
    static constexpr uint16_t kColorDotFail = Colors::kDanger;  // failed-endpoint dot
    static constexpr uint16_t kColorDotOk = TFT_LIGHTGRAY;      // ok dot
    static constexpr uint16_t kColorDotUnknown = TFT_DARKGREY;  // not probed yet

    // -----------------------------------------------------------------------
    const NetworkStatus& status_;

    TapAction tap_;

    // Top of the (fixed-height) drawn content, vertically centred within
    // dimensions_ — see kContentH's comment.
    int16_t contentTop() const {
        return dimensions_.y + (static_cast<int16_t>(dimensions_.height) - kContentH) / 2;
    }

    // Cached state for dirty detection
    bool lastConnected_ = false;
    int8_t lastRssiBracket_ = -1;
    bool lastEndpointOk_[NetworkStatusService::kNumEndpoints] = {};
    bool lastEndpointProbed_[NetworkStatusService::kNumEndpoints] = {};
    bool lastInitialized_ = false;

    // -----------------------------------------------------------------------
    void drawWifi();
    void drawDotGrid();

    // Returns RGB565 color for the current wifi state
    uint16_t wifiColor() const;

    // RSSI bracket 0–4: number of filled wifi bars, also used for dirty detection
    int8_t rssiBracket() const;

    // True if any endpoint_ok/endpoint_probed flag differs from cached
    bool endpointsDirty() const;
};
