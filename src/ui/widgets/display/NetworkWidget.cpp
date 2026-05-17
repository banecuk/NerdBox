#include "NetworkWidget.h"

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

NetworkWidget::NetworkWidget(const WidgetInterface::Dimensions& dims,
                             uint32_t updateIntervalMs,
                             const NetworkStatus& status)
    : Widget(dims, updateIntervalMs),
      status_(status) {}

// ---------------------------------------------------------------------------
// drawStatic — background only; called once on first paint and after wipe
// ---------------------------------------------------------------------------

void NetworkWidget::drawStatic() {
    if (!isInitialized_ || !getLcd()) return;
    LGFX* lcd = getLcd();

    lcd->fillRect(dimensions_.x, dimensions_.y,
                  dimensions_.width, dimensions_.height, TFT_BLACK);

    // Vertical separator between wifi and globe sections
    const int16_t sepX = dimensions_.x + kSectionW;
    lcd->drawFastVLine(sepX, dimensions_.y + 3, dimensions_.height - 6, 0x2104);

    isStaticDrawn_ = true;
    // Reset cache so onDraw does a full repaint
    lastConnected_    = false;
    lastRssiBracket_  = -1;
    lastInternet_     = NetworkStatus::Internet::UNKNOWN;
    lastInitialized_  = false;
    clearDirty();
}

// ---------------------------------------------------------------------------
// onDraw
// ---------------------------------------------------------------------------

void NetworkWidget::onDraw(bool forceRedraw) {
    if (!getLcd() || !isStaticDrawn_) return;

    const bool      connected = status_.wifi_connected;
    const int8_t    bracket   = rssiBracket();
    const auto      internet  = status_.internet;

    const bool changed =
        forceRedraw           ||
        !lastInitialized_     ||
        connected != lastConnected_ ||
        bracket   != lastRssiBracket_ ||
        internet  != lastInternet_;

    if (!changed) return;

    drawWifi();
    drawGlobe();

    lastConnected_   = connected;
    lastRssiBracket_ = bracket;
    lastInternet_    = internet;
    lastInitialized_ = true;
}

// ---------------------------------------------------------------------------
// drawWifi — left section: 4 signal bars
// ---------------------------------------------------------------------------

void NetworkWidget::drawWifi() {
    LGFX* lcd = getLcd();
    if (!lcd) return;

    // Clear section
    lcd->fillRect(dimensions_.x, dimensions_.y, kSectionW, dimensions_.height, TFT_BLACK);

    const uint8_t  filled = rssiBars();
    const uint16_t active = wifiColor();
    const uint16_t dim    = 0x2104;  // dark grey for unfilled bars

    // Bars right-aligned in the section, centred vertically
    // Heights: 5, 9, 14, 20 px (increasing)
    const uint8_t barHeights[kBarCount] = {5, 9, 14, 20};

    // Total bars width
    const uint16_t totalBarsW = kBarCount * kBarWidth + (kBarCount - 1) * kBarGap;
    // Centre bars horizontally in section
    const int16_t barsStartX  = dimensions_.x + (kSectionW - totalBarsW) / 2;
    const int16_t baselineY   = dimensions_.y + dimensions_.height - kBarBaseY;

    for (uint8_t i = 0; i < kBarCount; ++i) {
        const int16_t  bx = barsStartX + i * (kBarWidth + kBarGap);
        const uint8_t  bh = barHeights[i];
        const int16_t  by = baselineY - bh;
        const uint16_t c  = (i < filled) ? active : dim;
        lcd->fillRect(bx, by, kBarWidth, bh, c);
    }
}

// ---------------------------------------------------------------------------
// drawGlobe — right section: circle + latitude lines + vertical axis
// ---------------------------------------------------------------------------

void NetworkWidget::drawGlobe() {
    LGFX* lcd = getLcd();
    if (!lcd) return;

    // Clear section (leave separator)
    const int16_t secX = dimensions_.x + kSectionW + kSepW;
    lcd->fillRect(secX, dimensions_.y,
                  dimensions_.width - kSectionW - kSepW, dimensions_.height, TFT_BLACK);

    const uint16_t c = internetColor();

    // Centre of globe in its section
    const int16_t cx = secX + (kSectionW / 2);
    const int16_t cy = dimensions_.y + dimensions_.height / 2;

    // Outer circle
    lcd->drawCircle(cx, cy, kGlobeR, c);

    // Three horizontal latitude lines at 40%, 0%, -40% of radius
    for (int8_t frac : {-4, 0, 4}) {
        const int16_t ly = cy + (frac * static_cast<int16_t>(kGlobeR)) / 10;
        // Chord half-width at this y: w = sqrt(r² - dy²)
        const int16_t dy  = ly - cy;
        const int16_t r2  = kGlobeR * kGlobeR;
        const int16_t dy2 = dy * dy;
        if (dy2 >= r2) continue;
        const int16_t hw = static_cast<int16_t>(sqrtf(static_cast<float>(r2 - dy2)));
        lcd->drawFastHLine(cx - hw + 1, ly, hw * 2 - 2, c);
    }

    // Vertical axis
    lcd->drawFastVLine(cx, cy - kGlobeR + 1, kGlobeR * 2 - 2, c);
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

uint8_t NetworkWidget::rssiBars() const {
    if (!status_.wifi_connected) return 0;
    const int8_t r = status_.rssi;
    if (r > -65) return 4;
    if (r > -75) return 3;
    if (r > -85) return 2;
    return 1;
}

uint16_t NetworkWidget::wifiColor() const {
    if (!status_.wifi_connected) return 0x2104;
    const int8_t r = status_.rssi;
    if (r > -65) return TFT_WHITE;
    if (r > -75) return 0xFFE0;  // yellow
    if (r > -85) return 0xFD20;  // orange
    return 0xF800;               // red
}

uint16_t NetworkWidget::internetColor() const {
    using I = NetworkStatus::Internet;
    switch (status_.internet) {
        case I::OK:       return 0x07E0;  // green
        case I::DEGRADED: return 0xFFE0;  // yellow
        case I::DOWN:     return 0xF800;  // red
        default:          return 0x2104;  // dark grey — UNKNOWN
    }
}

int8_t NetworkWidget::rssiBracket() const {
    if (!status_.wifi_connected) return 0;
    const int8_t r = status_.rssi;
    if (r > -65) return 4;
    if (r > -75) return 3;
    if (r > -85) return 2;
    return 1;
}

bool NetworkWidget::handleTouch(uint16_t /*x*/, uint16_t /*y*/) {
    return false;
}
