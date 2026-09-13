#include "NetworkWidget.h"

#include "services/wifiScan/WifiScanMath.h"

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

NetworkWidget::NetworkWidget(const WidgetInterface::Dimensions& dims, uint32_t updateIntervalMs,
                             const NetworkStatus& status, EventType action,
                             ActionCallback callback)
    : Widget(dims, updateIntervalMs),
      status_(status),
      action_(action),
      callback_(std::move(callback)) {}

// ---------------------------------------------------------------------------
// drawStatic — background only; called once on first paint and after wipe
// ---------------------------------------------------------------------------

void NetworkWidget::onDrawStatic() {
    LGFX* lcd = getLcd();

    lcd->fillRect(dimensions_.x, dimensions_.y, dimensions_.width, dimensions_.height, TFT_BLACK);

    // Separator between wifi and globe sections
    const int16_t sepX = dimensions_.x + kWifiSectionW;
    lcd->drawFastVLine(sepX, contentTop() + 3, kContentH - 6, Colors::kHairline);

    // Reset cache so onDraw does a full repaint
    lastConnected_ = false;
    lastRssiBracket_ = -1;
    lastInternet_ = NetworkStatus::Internet::UNKNOWN;
    for (uint8_t i = 0; i < NetworkStatusService::kNumEndpoints; ++i)
        lastEndpointOk_[i] = false;
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
    const auto internet = status_.internet;

    const bool changed = forceRedraw || !lastInitialized_ || connected != lastConnected_ ||
                         bracket != lastRssiBracket_ || internet != lastInternet_ ||
                         endpointsDirty();

    if (!changed)
        return;

    drawWifi();
    drawGlobe();
    drawDotGrid();

    lastConnected_ = connected;
    lastRssiBracket_ = bracket;
    lastInternet_ = internet;
    for (uint8_t i = 0; i < NetworkStatusService::kNumEndpoints; ++i)
        lastEndpointOk_[i] = status_.endpoint_ok[i];
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
// drawGlobe — globe icon coloured by internet state
// ---------------------------------------------------------------------------

void NetworkWidget::drawGlobe() {
    LGFX* lcd = getLcd();
    if (!lcd)
        return;

    // Clear globe section (between separator and dot section)
    const int16_t secX = dimensions_.x + kWifiSectionW + kSepW;
    lcd->fillRect(secX, contentTop(), kGlobeSectionW, kContentH, TFT_BLACK);

    const uint16_t c = internetColor();

    // Centre globe in its section
    const int16_t cx = secX + kGlobeSectionW / 2;
    const int16_t cy = contentTop() + kContentH / 2;

    // Outer circle — anti-aliased ring (see docs-local/03-visual-ux.md V5):
    // a filled AA disc, then a black AA disc one pixel smaller punched out of
    // its centre. Safe on this write-only panel the same way ButtonWidget's
    // fill is (both AA edges blend against readRect()'s hard-coded black
    // stub, which matches this section's actual black background).
    lcd->fillSmoothCircle(cx, cy, kGlobeR, c);
    lcd->fillSmoothCircle(cx, cy, kGlobeR - 1, TFT_BLACK);

    // Three horizontal latitude lines at 40%, 0%, -40% of radius
    for (int8_t frac : {-4, 0, 4}) {
        const int16_t ly = cy + (frac * static_cast<int16_t>(kGlobeR)) / 10;
        const int16_t dy = ly - cy;
        const int16_t r2 = kGlobeR * kGlobeR;
        const int16_t dy2 = dy * dy;
        if (dy2 >= r2)
            continue;
        const int16_t hw = static_cast<int16_t>(sqrtf(static_cast<float>(r2 - dy2)));
        lcd->drawWideLine(cx - hw + 1, ly, cx + hw - 2, ly, 0.6f, c);
    }

    // Vertical axis
    lcd->drawWideLine(cx, cy - kGlobeR + 1, cx, cy + kGlobeR - 2, 0.6f, c);
}

// ---------------------------------------------------------------------------
// drawDotGrid — 4 columns × 2 rows, one dot per endpoint
//              white = OK, red = failed/unknown
// ---------------------------------------------------------------------------

void NetworkWidget::drawDotGrid() {
    LGFX* lcd = getLcd();
    if (!lcd)
        return;

    // Section starts after globe section
    const int16_t secX = dimensions_.x + kWifiSectionW + kSepW + kGlobeSectionW;
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
            const uint16_t c = status_.endpoint_ok[idx] ? kColorDotOk : kColorDotFail;
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

uint16_t NetworkWidget::internetColor() const {
    using I = NetworkStatus::Internet;
    switch (status_.internet) {
        case I::OK:
            return kColorOk;
        case I::WARNING:
            return kColorWarning;
        case I::DEGRADED:
            return kColorDegraded;
        case I::DOWN:
            return kColorDown;
        default:
            return kColorUnknown;
    }
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
        if (status_.endpoint_ok[i] != lastEndpointOk_[i])
            return true;
    }
    return false;
}

bool NetworkWidget::handleTouch(uint16_t /*x*/, uint16_t /*y*/) {
    if (!callback_)
        return false;
    callback_(action_);
    return true;
}
