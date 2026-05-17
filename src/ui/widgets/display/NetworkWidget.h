#pragma once

#include "services/network/NetworkStatus.h"
#include "ui/widgets/base/Widget.h"

// Compact status bar widget — sits left of the clock in the bottom strip.
//
// Layout (148 × 24 px):
//
//   [  WiFi bars + icon  68px  ] [ 1px sep ] [  Globe  68px  ] [ 11px pad ]
//
// WiFi section: 4-bar signal strength indicator with a small antenna icon.
// Globe section: internet reachability state coloured green/yellow/red/grey.
//
// Redraws only when RSSI bracket, wifi_connected, or internet state changes.
class NetworkWidget : public Widget {
public:
    NetworkWidget(const WidgetInterface::Dimensions& dims,
                  uint32_t updateIntervalMs,
                  const NetworkStatus& status);

    void drawStatic() override;
    bool handleTouch(uint16_t x, uint16_t y) override;

protected:
    void onDraw(bool forceRedraw) override;

private:
    // -----------------------------------------------------------------------
    // Layout
    // -----------------------------------------------------------------------
    static constexpr uint16_t kSectionW  = 68;  // each half
    static constexpr uint16_t kSepW      = 1;
    static constexpr uint16_t kPadRight  = 11;

    // Bar geometry (WiFi signal bars)
    static constexpr uint8_t kBarCount   = 4;
    static constexpr uint8_t kBarWidth   = 6;
    static constexpr uint8_t kBarGap     = 3;
    static constexpr uint8_t kBarMaxH    = 18;   // tallest bar height
    static constexpr uint8_t kBarBaseY   = 2;    // bottom margin from widget bottom

    // Globe geometry
    static constexpr uint8_t kGlobeR     = 9;    // radius px

    // -----------------------------------------------------------------------
    const NetworkStatus& status_;

    // Cached state for dirty detection
    bool     lastConnected_ = false;
    int8_t   lastRssiBracket_ = -1;  // 0–3, derived from RSSI dBm
    NetworkStatus::Internet lastInternet_ = NetworkStatus::Internet::UNKNOWN;
    bool     lastInitialized_ = false;

    // -----------------------------------------------------------------------
    void drawWifi();
    void drawGlobe();

    // Returns 0–4: number of filled bars for the current RSSI
    uint8_t rssiBars() const;

    // Returns RGB565 color for the current wifi state
    uint16_t wifiColor() const;

    // Returns RGB565 color for the current internet state
    uint16_t internetColor() const;

    // RSSI bracket 0–3 (poor/weak/ok/good) for dirty detection
    int8_t rssiBracket() const;
};
