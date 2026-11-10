#include "NetworkWidget.h"

#include "services/wifiScan/WifiScanMath.h"

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

NetworkWidget::NetworkWidget(const WidgetInterface::Dimensions& dims, uint32_t updateIntervalMs,
                             const NetworkStatus& status, TapAction tap)
    : Widget(dims, updateIntervalMs), status_(status), tap_(std::move(tap)) {}

// ---------------------------------------------------------------------------
// drawStatic — background only; called once on first paint and after wipe
// ---------------------------------------------------------------------------

void NetworkWidget::onDrawStatic() {
    LGFX* lcd = getLcd();

    lcd->fillRect(dimensions_.x, dimensions_.y, dimensions_.width, dimensions_.height, TFT_BLACK);

    // Separator between wifi and dot sections
    const int16_t sepX = dimensions_.x + kWifiSectionW;
    lcd->drawFastVLine(sepX, contentTop() + 3, kContentH - 6, Colors::kHairline);

    // Reset cache so onDraw does a full repaint
    lastConnected_ = false;
    lastRssiBracket_ = -1;
    for (uint8_t i = 0; i < NetworkStatusService::kNumEndpoints; ++i) {
        lastEndpointOk_[i] = false;
        lastEndpointProbed_[i] = false;
    }
    lastInitialized_ = false;
}

// ---------------------------------------------------------------------------
// onDraw
// ---------------------------------------------------------------------------

void NetworkWidget::onDraw(bool forceRedraw) {
    if (!getLcd() || !isStaticDrawn_)
        return;

    const bool connected = status_.wifi_connected;
    const int8_t bracket = rssiBracket();

    const bool changed = forceRedraw || !lastInitialized_ || connected != lastConnected_ ||
                         bracket != lastRssiBracket_ || endpointsDirty();

    if (!changed)
        return;

    drawWifi();
    drawDotGrid();

    lastConnected_ = connected;
    lastRssiBracket_ = bracket;
    for (uint8_t i = 0; i < NetworkStatusService::kNumEndpoints; ++i) {
        lastEndpointOk_[i] = status_.endpoint_ok[i];
        lastEndpointProbed_[i] = status_.endpoint_probed[i];
    }
    lastInitialized_ = true;
}

// ---------------------------------------------------------------------------
// drawWifi — left section: 4 signal bars
// ---------------------------------------------------------------------------

void NetworkWidget::drawWifi() {
    LGFX* lcd = getLcd();
    if (!lcd)
        return;

    lcd->fillRect(dimensions_.x, contentTop(), kWifiSectionW, kContentH, TFT_BLACK);

    const uint8_t filled = rssiBracket();
    const uint16_t active = wifiColor();
    const uint16_t dim = Colors::kHairline;

    const uint8_t barHeights[kBarCount] = {5, 9, 14, 20};

    const uint16_t totalBarsW = kBarCount * kBarWidth + (kBarCount - 1) * kBarGap;
    const int16_t barsStartX = dimensions_.x + (kWifiSectionW - totalBarsW) / 2;
    const int16_t baselineY = contentTop() + kContentH - kBarBaseY;

    for (uint8_t i = 0; i < kBarCount; ++i) {
        const int16_t bx = barsStartX + i * (kBarWidth + kBarGap);
        const uint8_t bh = barHeights[i];
        const int16_t by = baselineY - bh;
        const uint16_t c = (i < filled) ? active : dim;
        lcd->fillRect(bx, by, kBarWidth, bh, c);
    }
}

// ---------------------------------------------------------------------------
// drawDotGrid — 4 columns × 2 rows, one dot per endpoint
//              white = OK, red = failed, dark grey = not probed yet
// ---------------------------------------------------------------------------

void NetworkWidget::drawDotGrid() {
    LGFX* lcd = getLcd();
    if (!lcd)
        return;

    // Section starts right after the separator
    const int16_t secX = dimensions_.x + kWifiSectionW + kSepW;
    lcd->fillRect(secX, contentTop(), kDotSectionW, kContentH, TFT_BLACK);

    // Centre the 4×2 grid within the dot section
    // Total grid width  = 4 cols, gap between centres = kDotSpacX
    // Total grid height = 2 rows, gap between centres = kDotSpacY
    const int16_t gridW = (kDotCols - 1) * kDotSpacX;
    const int16_t gridH = (kDotRows - 1) * kDotSpacY;
    const int16_t originX = secX + (kDotSectionW - gridW) / 2;
    const int16_t originY = contentTop() + (kContentH - gridH) / 2;

    for (uint8_t row = 0; row < kDotRows; ++row) {
        for (uint8_t col = 0; col < kDotCols; ++col) {
            const uint8_t idx = row * kDotCols + col;
            const int16_t cx = originX + col * kDotSpacX;
            const int16_t cy = originY + row * kDotSpacY;
            const uint16_t c = !status_.endpoint_probed[idx] ? kColorDotUnknown
                               : status_.endpoint_ok[idx]    ? kColorDotOk
                                                             : kColorDotFail;
            lcd->fillSmoothCircle(cx, cy, kDotR, c);
        }
    }
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

uint16_t NetworkWidget::wifiColor() const {
    if (!status_.wifi_connected)
        return Colors::kHairline;
    // Shared with WifiLinkWidget/WifiScanListWidget — see
    // docs-local/13-wifi-screen-plan.md §4.2 (the C2 duplication this closes).
    switch (WifiScanMath::signalTier(status_.rssi)) {
        case WifiScanMath::SignalTier::kStrong:
            return TFT_LIGHTGRAY;
        case WifiScanMath::SignalTier::kWarn:
            return kColorWarning;
        case WifiScanMath::SignalTier::kDegraded:
            return kColorDegraded;
        case WifiScanMath::SignalTier::kWeak:
            return kColorDown;
    }
    return kColorDown;
}

int8_t NetworkWidget::rssiBracket() const {
    if (!status_.wifi_connected)
        return 0;
    switch (WifiScanMath::signalTier(status_.rssi)) {
        case WifiScanMath::SignalTier::kStrong:
            return 4;
        case WifiScanMath::SignalTier::kWarn:
            return 3;
        case WifiScanMath::SignalTier::kDegraded:
            return 2;
        case WifiScanMath::SignalTier::kWeak:
            return 1;
    }
    return 1;
}

bool NetworkWidget::endpointsDirty() const {
    for (uint8_t i = 0; i < NetworkStatusService::kNumEndpoints; ++i) {
        if (status_.endpoint_ok[i] != lastEndpointOk_[i] ||
            status_.endpoint_probed[i] != lastEndpointProbed_[i])
            return true;
    }
    return false;
}

bool NetworkWidget::handleTouch(uint16_t /*x*/, uint16_t /*y*/) {
    return tap_.fire();
}
