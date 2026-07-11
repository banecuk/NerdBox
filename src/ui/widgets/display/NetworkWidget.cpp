#include "NetworkWidget.h"

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

NetworkWidget::NetworkWidget(const WidgetInterface::Dimensions& dims, uint32_t updateIntervalMs,
                             const NetworkStatus& status)
    : Widget(dims, updateIntervalMs), status_(status) {}

// ---------------------------------------------------------------------------
// drawStatic — background only; called once on first paint and after wipe
// ---------------------------------------------------------------------------

void NetworkWidget::onDrawStatic() {
    LGFX* lcd = getLcd();

    lcd->fillRect(dimensions_.x, dimensions_.y, dimensions_.width, dimensions_.height, TFT_BLACK);

    // Separator between wifi and globe sections
    const int16_t sepX = dimensions_.x + kWifiSectionW;
    lcd->drawFastVLine(sepX, dimensions_.y + 3, dimensions_.height - 6, Colors::kHairline);

    // Reset cache so onDraw does a full repaint
    lastConnected_ = false;
    lastRssiBracket_ = -1;
    lastInternet_ = NetworkStatus::Internet::UNKNOWN;
    for (uint8_t i = 0; i < 6; ++i)
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
    for (uint8_t i = 0; i < 6; ++i)
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

    lcd->fillRect(dimensions_.x, dimensions_.y, kWifiSectionW, dimensions_.height, TFT_BLACK);

    const uint8_t filled = rssiBracket();
    const uint16_t active = wifiColor();
    const uint16_t dim = Colors::kHairline;

    const uint8_t barHeights[kBarCount] = {5, 9, 14, 20};

    const uint16_t totalBarsW = kBarCount * kBarWidth + (kBarCount - 1) * kBarGap;
    const int16_t barsStartX = dimensions_.x + (kWifiSectionW - totalBarsW) / 2;
    const int16_t baselineY = dimensions_.y + dimensions_.height - kBarBaseY;

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
    lcd->fillRect(secX, dimensions_.y, kGlobeSectionW, dimensions_.height, TFT_BLACK);

    const uint16_t c = internetColor();

    // Centre globe in its section
    const int16_t cx = secX + kGlobeSectionW / 2;
    const int16_t cy = dimensions_.y + dimensions_.height / 2;

    // Outer circle
    lcd->drawCircle(cx, cy, kGlobeR, c);

    // Three horizontal latitude lines at 40%, 0%, -40% of radius
    for (int8_t frac : {-4, 0, 4}) {
        const int16_t ly = cy + (frac * static_cast<int16_t>(kGlobeR)) / 10;
        const int16_t dy = ly - cy;
        const int16_t r2 = kGlobeR * kGlobeR;
        const int16_t dy2 = dy * dy;
        if (dy2 >= r2)
            continue;
        const int16_t hw = static_cast<int16_t>(sqrtf(static_cast<float>(r2 - dy2)));
        lcd->drawFastHLine(cx - hw + 1, ly, hw * 2 - 2, c);
    }

    // Vertical axis
    lcd->drawFastVLine(cx, cy - kGlobeR + 1, kGlobeR * 2 - 2, c);
}

// ---------------------------------------------------------------------------
// drawDotGrid — 3 columns × 2 rows, one dot per endpoint
//              white = OK, red = failed/unknown
// ---------------------------------------------------------------------------

void NetworkWidget::drawDotGrid() {
    LGFX* lcd = getLcd();
    if (!lcd)
        return;

    // Section starts after globe section
    const int16_t secX = dimensions_.x + kWifiSectionW + kSepW + kGlobeSectionW;
    lcd->fillRect(secX, dimensions_.y, kDotSectionW, dimensions_.height, TFT_BLACK);

    // Centre the 3×2 grid within the dot section
    // Total grid width  = 3 cols, gap between centres = kDotSpacX
    // Total grid height = 2 rows, gap between centres = kDotSpacY
    const int16_t gridW = (kDotCols - 1) * kDotSpacX;
    const int16_t gridH = (kDotRows - 1) * kDotSpacY;
    const int16_t originX = secX + (kDotSectionW - gridW) / 2;
    const int16_t originY = dimensions_.y + (dimensions_.height - gridH) / 2;

    for (uint8_t row = 0; row < kDotRows; ++row) {
        for (uint8_t col = 0; col < kDotCols; ++col) {
            const uint8_t idx = row * kDotCols + col;
            const int16_t cx = originX + col * kDotSpacX;
            const int16_t cy = originY + row * kDotSpacY;
            const uint16_t c = status_.endpoint_ok[idx] ? kColorDotOk : kColorDotFail;
            lcd->fillCircle(cx, cy, kDotR, c);
        }
    }
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

uint16_t NetworkWidget::wifiColor() const {
    if (!status_.wifi_connected)
        return Colors::kHairline;
    const int8_t r = status_.rssi;
    if (r > -65)
        return TFT_LIGHTGRAY;
    if (r > -75)
        return kColorWarning;  // yellow
    if (r > -85)
        return kColorDegraded;  // orange
    return kColorDown;          // red
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
    const int8_t r = status_.rssi;
    if (r > -65)
        return 4;
    if (r > -75)
        return 3;
    if (r > -85)
        return 2;
    return 1;
}

bool NetworkWidget::endpointsDirty() const {
    for (uint8_t i = 0; i < 6; ++i) {
        if (status_.endpoint_ok[i] != lastEndpointOk_[i])
            return true;
    }
    return false;
}

bool NetworkWidget::handleTouch(uint16_t /*x*/, uint16_t /*y*/) {
    return false;
}
