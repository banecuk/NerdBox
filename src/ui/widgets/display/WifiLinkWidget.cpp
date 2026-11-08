#include "WifiLinkWidget.h"

#include <cstdio>
#include <cstring>

#include "services/wifiScan/WifiScanMath.h"
#include "ui/core/Colors.h"
#include "ui/core/UiText.h"
#include "ui/resources/FontRegistry.h"

namespace {
constexpr uint16_t kBgColor = TFT_BLACK;
constexpr uint16_t kLabelColor = TFT_DARKGREY;
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

WifiLinkWidget::WifiLinkWidget(const WidgetInterface::Dimensions& dims, uint32_t updateIntervalMs,
                               const NetworkStatus& netStatus, const WifiScanData& wifiScan,
                               const AppSettings& config)
    : Widget(dims, updateIntervalMs), netStatus_(netStatus), wifiScan_(wifiScan), config_(config) {}

void WifiLinkWidget::onDrawStatic() {
    LGFX* lcd = getLcd();
    lcd->fillRect(dimensions_.x, dimensions_.y, dimensions_.width, dimensions_.height, kBgColor);
    lastConnected_ = false;
    lastIp_[0] = lastGateway_[0] = lastMask_[0] = lastDns_[0] = lastMac_[0] = lastHostname_[0] =
        lastSsid_[0] = '\0';
    lastAuth_ = lastChannel_ = lastNeighbourCount_ = 0xFF;
    lastNeighbourAvailable_ = false;
    lastRssi_ = 127;
    rssiTrace_.clear();
}

void WifiLinkWidget::onDraw(bool forceRedraw) {
    LGFX* lcd = getLcd();
    if (!lcd)
        return;

    LinkInfo link;
    netStatus_.copyLink(link);

    if (!link.connected) {
        if (forceRedraw || lastConnected_) {
            drawDisconnected();
        }
        lastConnected_ = false;
        return;
    }

    const bool structural = forceRedraw || !lastConnected_;
    if (structural) {
        // Structural change (screen entry or a fresh connect) — the whole
        // card was just cleared by drawDisconnected()/onDrawStatic(), so
        // force every cached string to redraw below.
        lastIp_[0] = lastGateway_[0] = lastMask_[0] = lastDns_[0] = lastMac_[0] = lastHostname_[0] =
            lastSsid_[0] = '\0';
        lastAuth_ = lastChannel_ = lastNeighbourCount_ = 0xFF;
        lastNeighbourAvailable_ = false;
        lastRssi_ = 127;
        rssiTrace_.clear();
    }

    rssiTrace_.push(link.rssi);

    drawTopRow(link, structural);
    // Redraw on every push: the trace scrolls, so each new sample changes the
    // image even when RSSI is steady. (Gating this on an RSSI change froze
    // the trace in place — a dip never scrolled away, B51.) drawTrace()
    // overdraws column by column instead of clear-then-draw, so it doesn't
    // flash.
    drawTrace();
    drawDetailsStrip(link, structural);

    lastRssi_ = link.rssi;

    lastConnected_ = true;
}

void WifiLinkWidget::drawDisconnected() {
    LGFX* lcd = getLcd();
    lcd->fillRect(dimensions_.x, dimensions_.y, dimensions_.width, kHairlineY, kBgColor);

    Fonts::loadMetric(lcd);
    lcd->setTextColor(Colors::kDanger, kBgColor);
    lcd->setTextDatum(MC_DATUM);
    lcd->drawString(UiText::kNoLink, dimensions_.x + dimensions_.width / 2,
                    dimensions_.y + kHairlineY / 2);
    Fonts::unload(lcd);

    lastIp_[0] = lastGateway_[0] = lastMask_[0] = lastDns_[0] = lastMac_[0] = lastHostname_[0] =
        lastSsid_[0] = '\0';
    rssiTrace_.clear();
}

// ---------------------------------------------------------------------------
// Top row: SSID + subtitle (left), bars/dBm + trace/quality (right)
// ---------------------------------------------------------------------------

void WifiLinkWidget::drawTopRow(const LinkInfo& link, bool forceRedraw) {
    LGFX* lcd = getLcd();
    const uint16_t rightColX = dimensions_.x + dimensions_.width - 190;
    const uint16_t row1Y = dimensions_.y + 8;
    const uint16_t row2Y = dimensions_.y + 44;
    const WifiScanMath::SignalTier tier = WifiScanMath::signalTier(link.rssi);
    const uint16_t tierC = tierColor(tier);

    // SSID — redrawn only when it changes (reconnect to a different network).
    if (forceRedraw || strcmp(link.ssid, lastSsid_) != 0) {
        lcd->fillRect(dimensions_.x, row1Y, rightColX - dimensions_.x, 34, kBgColor);
        Fonts::loadMetric(lcd);
        lcd->setTextColor(kValueColor, kBgColor);
        lcd->setTextDatum(TL_DATUM);
        lcd->drawString(link.ssid[0] ? link.ssid : "(hidden)", dimensions_.x + 10, row1Y);
        Fonts::unload(lcd);
        strncpy(lastSsid_, link.ssid, sizeof(lastSsid_) - 1);
        lastSsid_[sizeof(lastSsid_) - 1] = '\0';
    }

    // Subtitle: "WPA2 · ch 6 · N neighbours here" — redrawn only when one of
    // its inputs actually changes (auth/channel barely ever do; the
    // neighbour count moves at most once per rescan). Without this gate the
    // black-clear-then-redraw happened every tick regardless, which visibly
    // flashed on real hardware even though the text was identical.
    //
    // wifiScan_.sameChannelCount is only meaningful once a scan has actually
    // completed (freshness.available()) — it defaults to 0, so showing
    // "0 neighbours here" before that would misreport "confirmed no
    // neighbours" as a scan result that hasn't happened yet.
    const bool neighbourAvailable = wifiScan_.freshness.available();
    const bool subtitleChanged = forceRedraw || link.auth != lastAuth_ ||
                                 link.channel != lastChannel_ ||
                                 wifiScan_.sameChannelCount != lastNeighbourCount_ ||
                                 neighbourAvailable != lastNeighbourAvailable_;
    if (subtitleChanged) {
        lcd->fillRect(dimensions_.x, row2Y, rightColX - dimensions_.x, 26, kBgColor);
        char prefix[24];
        snprintf(prefix, sizeof(prefix), "%s \xc2\xb7 ch %u \xc2\xb7 ",
                 WifiScanMath::authName(link.auth), link.channel);
        char suffix[24];
        if (neighbourAvailable) {
            snprintf(suffix, sizeof(suffix), "%u neighbour%s here", wifiScan_.sameChannelCount,
                     wifiScan_.sameChannelCount == 1 ? "" : "s");
        } else {
            snprintf(suffix, sizeof(suffix), "scanning...");
        }

        Fonts::loadLabel(lcd);
        lcd->setTextDatum(TL_DATUM);
        lcd->setTextColor(kLabelColor, kBgColor);
        lcd->drawString(prefix, dimensions_.x + 10, row2Y + 6);
        const uint16_t prefixW = static_cast<uint16_t>(lcd->textWidth(prefix));
        const uint16_t suffixColor = !neighbourAvailable               ? kLabelColor
                                     : wifiScan_.sameChannelCount >= 6 ? Colors::kDanger
                                     : wifiScan_.sameChannelCount >= 3 ? Colors::kWarn
                                                                       : kLabelColor;
        lcd->setTextColor(suffixColor, kBgColor);
        lcd->drawString(suffix, dimensions_.x + 10 + prefixW, row2Y + 6);
        Fonts::unload(lcd);

        lastAuth_ = link.auth;
        lastChannel_ = link.channel;
        lastNeighbourCount_ = wifiScan_.sameChannelCount;
        lastNeighbourAvailable_ = neighbourAvailable;
    }

    // Bars + dBm — right column, row 1. Same flash: only redraw when the
    // RSSI (and therefore the bars/colour/number) actually moved.
    if (forceRedraw || link.rssi != lastRssi_) {
        lcd->fillRect(rightColX, row1Y, dimensions_.width - (rightColX - dimensions_.x), 34,
                      kBgColor);
        static constexpr uint8_t kBarCount = 4;
        static constexpr uint8_t kBarWidth = 8;
        static constexpr uint8_t kBarGap = 4;
        static constexpr uint8_t kBarHeights[kBarCount] = {8, 14, 20, 26};
        const uint8_t filled = tier == WifiScanMath::SignalTier::kStrong     ? 4
                               : tier == WifiScanMath::SignalTier::kWarn     ? 3
                               : tier == WifiScanMath::SignalTier::kDegraded ? 2
                                                                             : 1;
        const uint16_t barsBaselineY = row1Y + 30;
        for (uint8_t i = 0; i < kBarCount; ++i) {
            const uint16_t bx = rightColX + i * (kBarWidth + kBarGap);
            const uint8_t bh = kBarHeights[i];
            lcd->fillRect(bx, barsBaselineY - bh, kBarWidth, bh,
                          (i < filled) ? tierC : Colors::kHairline);
        }

        char dbm[12];
        snprintf(dbm, sizeof(dbm), "%d dBm", link.rssi);
        Fonts::loadLabel(lcd);
        lcd->setTextColor(kLabelColor, kBgColor);
        lcd->setTextDatum(TR_DATUM);
        lcd->drawString(dbm, dimensions_.x + dimensions_.width - 8, row1Y + 10);
        Fonts::unload(lcd);
    }
}

// ---------------------------------------------------------------------------
// RSSI trace + quality — right column, row 2
// ---------------------------------------------------------------------------

void WifiLinkWidget::drawTrace() {
    LGFX* lcd = getLcd();
    const uint16_t row2Y = dimensions_.y + 44;
    const uint16_t traceX = dimensions_.x + dimensions_.width - 190;
    const uint16_t traceW = kTraceLen * kTraceColW;
    const uint16_t traceH = 22;
    const uint16_t traceY = row2Y + 2;
    const uint16_t qualityX = dimensions_.x + dimensions_.width - 8;

    const size_t count = rssiTrace_.size();
    const size_t offset = kTraceLen - count;
    const WifiScanMath::SignalTier latestTier =
        count > 0 ? WifiScanMath::signalTier(rssiTrace_.at(count - 1))
                  : WifiScanMath::SignalTier::kWeak;
    const uint16_t traceColor = tierColor(latestTier);

    for (size_t col = 0; col < kTraceLen; ++col) {
        const uint16_t x = traceX + static_cast<uint16_t>(col) * kTraceColW;
        if (col < offset) {
            lcd->fillRect(x, traceY, kTraceColW, traceH, kBgColor);
            continue;
        }
        const int8_t sample = rssiTrace_.at(col - offset);
        const int16_t clamped =
            sample < kTraceMinDbm ? kTraceMinDbm : (sample > kTraceMaxDbm ? kTraceMaxDbm : sample);
        const uint16_t h = static_cast<uint16_t>(
            ((clamped - kTraceMinDbm) * (traceH - 1)) / (kTraceMaxDbm - kTraceMinDbm) + 1);
        // Overdraw the whole column: background above the bar (plus the
        // 1px gap column), then the bar — no full-area clear, no flash.
        lcd->fillRect(x, traceY, kTraceColW, traceH - h, kBgColor);
        lcd->fillRect(x + kTraceColW - 1, traceY + traceH - h, 1, h, kBgColor);
        lcd->fillRect(x, traceY + traceH - h, kTraceColW - 1, h, traceColor);
    }

    const uint8_t quality = count > 0 ? WifiScanMath::rssiToQuality(rssiTrace_.at(count - 1)) : 0;
    char qualityStr[6];
    snprintf(qualityStr, sizeof(qualityStr), "%u%%", quality);
    // Clear the quality box first: it's right-aligned, so "100%"→"99%"
    // otherwise leaves the old leading digit behind (B55).
    const uint16_t qualityBoxX = traceX + traceW + 1;
    lcd->fillRect(qualityBoxX, traceY, qualityX - qualityBoxX, traceH, kBgColor);
    Fonts::loadLabel(lcd);
    lcd->setTextColor(kLabelColor, kBgColor);
    lcd->setTextDatum(TR_DATUM);
    lcd->drawString(qualityStr, qualityX, traceY + 2);
    Fonts::unload(lcd);
}

// ---------------------------------------------------------------------------
// Details strip — IP/GW/MASK (row 1), DNS/MAC/HOST (row 2)
// ---------------------------------------------------------------------------

void WifiLinkWidget::drawDetailsStrip(const LinkInfo& link, bool forceRedraw) {
    LGFX* lcd = getLcd();
    const uint16_t rowY1 = dimensions_.y + kCardH + 4;
    const uint16_t rowY2 = dimensions_.y + kCardH + 22;
    static constexpr uint16_t kColX[3] = {8, 170, 330};

    auto drawField = [&](uint16_t x, uint16_t y, const char* label, const char* value, char* cache,
                         size_t cacheSize) {
        if (!forceRedraw && strcmp(value, cache) == 0)
            return;
        strncpy(cache, value, cacheSize - 1);
        cache[cacheSize - 1] = '\0';

        char buf[80];
        snprintf(buf, sizeof(buf), "%s %s", label, value);
        lcd->fillRect(dimensions_.x + x, y, x == kColX[2] ? dimensions_.width - x - 8 : 160, 16,
                      kBgColor);
        Fonts::loadLabel(lcd);
        lcd->setTextColor(kValueColor, kBgColor);
        lcd->setTextDatum(TL_DATUM);
        lcd->drawString(buf, dimensions_.x + x, y);
        Fonts::unload(lcd);
    };

    drawField(kColX[0], rowY1, "IP", link.ip, lastIp_, sizeof(lastIp_));
    drawField(kColX[1], rowY1, "GW", link.gateway, lastGateway_, sizeof(lastGateway_));
    drawField(kColX[2], rowY1, "MASK", link.mask, lastMask_, sizeof(lastMask_));
    drawField(kColX[0], rowY2, "DNS", link.dns, lastDns_, sizeof(lastDns_));
    drawField(kColX[1], rowY2, "MAC", link.mac, lastMac_, sizeof(lastMac_));
    drawField(kColX[2], rowY2, "HOST", link.hostname, lastHostname_, sizeof(lastHostname_));

    if (forceRedraw) {
        lcd->drawFastHLine(dimensions_.x, dimensions_.y + kHairlineY, dimensions_.width,
                           Colors::kHairline);
    }
}
