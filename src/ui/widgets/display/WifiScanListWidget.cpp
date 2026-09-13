#include "WifiScanListWidget.h"

#include <cstdio>
#include <cstring>

#include "services/wifiScan/WifiScanMath.h"
#include "ui/core/Colors.h"
#include "ui/resources/FontRegistry.h"

namespace {
constexpr uint16_t kBgColor = TFT_BLACK;
constexpr uint16_t kHeaderColor = TFT_DARKGREY;
constexpr uint16_t kValueColor = TFT_WHITE;

uint16_t tierColor(WifiScanMath::SignalTier tier) {
    switch (tier) {
        case WifiScanMath::SignalTier::kStrong:
            return TFT_LIGHTGRAY;
        case WifiScanMath::SignalTier::kWarn:
            return Colors::kWarn;
        case WifiScanMath::SignalTier::kDegraded:
            return Colors::kDegraded;
        case WifiScanMath::SignalTier::kWeak:
            return Colors::kDanger;
    }
    return Colors::kDanger;
}
}  // namespace

WifiScanListWidget::WifiScanListWidget(const WidgetInterface::Dimensions& dims,
                                       uint32_t updateIntervalMs, const WifiScanData& data)
    : Widget(dims, updateIntervalMs), data_(data) {}

void WifiScanListWidget::onDrawStatic() {
    getLcd()->fillRect(dimensions_.x, dimensions_.y, dimensions_.width, dimensions_.height,
                       kBgColor);
    for (auto& row : lastRows_) {
        row = RowCache{};
    }
    lastState_ = WifiScanData::State::IDLE;
    lastCount_ = 0;
    lastFreshnessMs_ = 0;
    lastDrawnAgeS_ = 0;
    drawHeader();
}

void WifiScanListWidget::onDraw(bool forceRedraw) {
    if (!getLcd())
        return;

    const bool structuralChange = forceRedraw || data_.state != lastState_;
    const bool freshnessChanged = data_.freshness.lastUpdateMs() != lastFreshnessMs_;

    // "scanned Ns ago" ticks up every second even though nothing else about
    // the scan changed — redraw the header whenever that display value would
    // differ, not just on a structural/freshness change, or it goes stale on
    // screen (e.g. stuck showing "0s ago").
    const unsigned long ageS =
        data_.freshness.available() ? (millis() - data_.freshness.lastUpdateMs()) / 1000 : 0;
    if (structuralChange || freshnessChanged || ageS != lastDrawnAgeS_) {
        drawHeader();
        lastDrawnAgeS_ = ageS;
    }

    // The empty-state body doesn't change unless the state/freshness does —
    // redrawing it every tick just repaints identical pixels.
    const bool bodyChange = structuralChange || freshnessChanged;

    if (data_.state == WifiScanData::State::FAILED) {
        if (bodyChange)
            drawEmptyState("SCAN FAILED", Colors::kDanger);
    } else if (!data_.freshness.available()) {
        // Nothing has ever completed yet, whatever the current state is.
        if (bodyChange)
            drawEmptyState("SCANNING...", TFT_DARKGREY);
    } else if (data_.count == 0) {
        if (bodyChange)
            drawEmptyState("NO NETWORKS FOUND", TFT_DARKGREY);
    } else {
        uint8_t ourChannel = 0;
        for (uint8_t i = 0; i < data_.count; ++i) {
            if (data_.entries[i].isCurrent) {
                ourChannel = data_.entries[i].channel;
                break;
            }
        }
        for (uint8_t r = 0; r < kRows; ++r) {
            if (r < data_.count) {
                drawRow(r, data_.entries[r], structuralChange, ourChannel);
            } else {
                clearRow(r);
            }
        }
    }

    lastState_ = data_.state;
    lastCount_ = data_.count;
    lastFreshnessMs_ = data_.freshness.lastUpdateMs();
    lastUpdateTimeMs_ = millis();
    clearDirty();
}

// ---------------------------------------------------------------------------
// Header — column labels + live "scanned Ns ago" / "scanning..." status
// ---------------------------------------------------------------------------

void WifiScanListWidget::drawHeader() {
    LGFX* lcd = getLcd();
    lcd->fillRect(dimensions_.x, dimensions_.y, dimensions_.width, kHeaderH, kBgColor);

    char status[40];
    if (data_.state == WifiScanData::State::SCANNING) {
        snprintf(status, sizeof(status), "NEARBY \xc2\xb7 scanning...");
    } else if (data_.state == WifiScanData::State::FAILED) {
        snprintf(status, sizeof(status), "NEARBY \xc2\xb7 scan failed");
    } else if (data_.freshness.available()) {
        const unsigned long ageS = (millis() - data_.freshness.lastUpdateMs()) / 1000;
        snprintf(status, sizeof(status), "NEARBY \xc2\xb7 scanned %lus ago", ageS);
    } else {
        snprintf(status, sizeof(status), "NEARBY");
    }

    Fonts::loadLabel(lcd);
    lcd->setTextColor(kHeaderColor, kBgColor);
    lcd->setTextDatum(TL_DATUM);
    lcd->drawString(status, dimensions_.x + kColSsid, dimensions_.y + 2);
    lcd->drawString("CH", dimensions_.x + kColCh, dimensions_.y + 2);
    lcd->drawString("SEC", dimensions_.x + kColSec, dimensions_.y + 2);
    lcd->setTextDatum(TR_DATUM);
    lcd->drawString("RSSI", dimensions_.x + kColRssiRight, dimensions_.y + 2);
    Fonts::unload(lcd);
}

// ---------------------------------------------------------------------------
// Rows
// ---------------------------------------------------------------------------

void WifiScanListWidget::drawRow(uint8_t row, const WifiApEntry& entry, bool forceRedraw,
                                 uint8_t ourChannel) {
    RowCache& cache = lastRows_[row];
    const bool changed = forceRedraw || !cache.valid || strcmp(entry.ssid, cache.ssid) != 0 ||
                         entry.channel != cache.channel || entry.auth != cache.auth ||
                         entry.rssi != cache.rssi || entry.isCurrent != cache.isCurrent;
    if (!changed)
        return;

    LGFX* lcd = getLcd();
    const uint16_t y = dimensions_.y + kHeaderH + row * kRowH;
    lcd->fillRect(dimensions_.x, y, dimensions_.width, kRowH, kBgColor);

    const WifiScanMath::SignalTier tier = WifiScanMath::signalTier(entry.rssi);
    const uint16_t tierC = tierColor(tier);
    const uint8_t filled = tier == WifiScanMath::SignalTier::kStrong    ? 4
                          : tier == WifiScanMath::SignalTier::kWarn     ? 3
                          : tier == WifiScanMath::SignalTier::kDegraded ? 2
                                                                          : 1;

    // Bars.
    static constexpr uint8_t kBarHeights[4] = {4, 7, 10, 13};
    const uint16_t baseline = y + kRowH - 3;
    for (uint8_t i = 0; i < 4; ++i) {
        const uint16_t bx = dimensions_.x + kColBars + i * 5;
        const uint8_t bh = kBarHeights[i];
        lcd->fillRect(bx, baseline - bh, 3, bh, (i < filled) ? tierC : Colors::kHairline);
    }

    // SSID (+ current-AP dot).
    char ssidLabel[40];
    snprintf(ssidLabel, sizeof(ssidLabel), "%s", entry.ssid[0] ? entry.ssid : "(hidden)");
    Fonts::loadValue(lcd);
    lcd->setTextDatum(ML_DATUM);
    lcd->setTextColor(entry.ssid[0] ? kValueColor : Colors::kInactiveText, kBgColor);
    lcd->drawString(ssidLabel, dimensions_.x + kColSsid, y + kRowH / 2);
    if (entry.isCurrent) {
        const uint16_t textW = static_cast<uint16_t>(lcd->textWidth(ssidLabel));
        lcd->fillSmoothCircle(dimensions_.x + kColSsid + textW + 8, y + kRowH / 2, 3, Colors::kOk);
    }

    // Channel — tinted if it's a co-channel neighbour (not us).
    const bool coChannel = !entry.isCurrent && entry.channel == ourChannel;
    char chStr[4];
    snprintf(chStr, sizeof(chStr), "%u", entry.channel);
    lcd->setTextColor(coChannel ? Colors::kWarn : kValueColor, kBgColor);
    lcd->drawString(chStr, dimensions_.x + kColCh, y + kRowH / 2);

    // Security — OPEN in danger.
    const char* sec = WifiScanMath::authName(entry.auth);
    lcd->setTextColor(entry.auth == 0 ? Colors::kDanger : kValueColor, kBgColor);
    lcd->drawString(sec, dimensions_.x + kColSec, y + kRowH / 2);

    // RSSI — right-aligned, tinted by tier.
    char rssiStr[8];
    snprintf(rssiStr, sizeof(rssiStr), "%d", entry.rssi);
    lcd->setTextDatum(MR_DATUM);
    lcd->setTextColor(tierC, kBgColor);
    lcd->drawString(rssiStr, dimensions_.x + kColRssiRight, y + kRowH / 2);
    Fonts::unload(lcd);

    cache.valid = true;
    strncpy(cache.ssid, entry.ssid, sizeof(cache.ssid) - 1);
    cache.ssid[sizeof(cache.ssid) - 1] = '\0';
    cache.channel = entry.channel;
    cache.auth = entry.auth;
    cache.rssi = entry.rssi;
    cache.isCurrent = entry.isCurrent;
}

void WifiScanListWidget::clearRow(uint8_t row) {
    RowCache& cache = lastRows_[row];
    if (!cache.valid)
        return;
    LGFX* lcd = getLcd();
    const uint16_t y = dimensions_.y + kHeaderH + row * kRowH;
    lcd->fillRect(dimensions_.x, y, dimensions_.width, kRowH, kBgColor);
    cache = RowCache{};
}

void WifiScanListWidget::drawEmptyState(const char* message, uint16_t color) {
    LGFX* lcd = getLcd();
    const uint16_t y = dimensions_.y + kHeaderH;
    const uint16_t h = dimensions_.height - kHeaderH;
    lcd->fillRect(dimensions_.x, y, dimensions_.width, h, kBgColor);

    Fonts::loadLabel(lcd);
    lcd->setTextColor(color, kBgColor);
    lcd->setTextDatum(MC_DATUM);
    lcd->drawString(message, dimensions_.x + dimensions_.width / 2, y + h / 2);
    Fonts::unload(lcd);

    for (auto& row : lastRows_) {
        row = RowCache{};
    }
}
