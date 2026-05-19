#include "BrightnessWidget.h"

#include "core/resources/FontRegistry.h"

BrightnessWidget::BrightnessWidget(const WidgetInterface::Dimensions& dims,
                                   DisplayManager& displayManager)
    : Widget(dims, 0),
      displayManager_(displayManager) {
    buildSegments();
}

void BrightnessWidget::buildSegments() {
    using Cfg = AppConfig::internal::UiImpl;

    // Evenly divide available width across kSegmentCount segments.
    const uint16_t totalGaps = kGap * (kSegmentCount - 1);
    const uint16_t segW      = (dimensions_.width - totalGaps) / kSegmentCount;

    // Labels — one per level, ordered dim → bright.
    static constexpr const char* kLabels[kSegmentCount] = { "1", "2", "3", "4", "5" };

    // Active-state accent colours, graduating dim-blue → amber → white.
    // RGB565 values chosen so text (black) is readable on each.
    static constexpr uint16_t kColors[kSegmentCount] = {
        0x4208,   // 1 — dim blue-grey  (~#404040)
        0x7BCF,   // 2 — steel blue     (~#7090C8)
        0xFD20,   // 3 — amber          (~#FF6800)
        0xFF80,   // 4 — warm yellow    (~#FFB000)
        TFT_WHITE // 5 — full white
    };

    for (uint8_t i = 0; i < kSegmentCount; ++i) {
        segments_[i] = {
            static_cast<uint16_t>(dimensions_.x + i * (segW + kGap)),
            segW,
            Cfg::kBrightnessLevels[i],
            kLabels[i],
            kColors[i]
        };
    }
}

void BrightnessWidget::drawStatic() {
    if (!isInitialized_ || !getLcd())
        return;

    LGFX* lcd = getLcd();
    lcd->fillRect(dimensions_.x, dimensions_.y,
                  dimensions_.width, dimensions_.height, TFT_BLACK);

    Fonts::loadLabel(lcd);
    lcd->setTextColor(TFT_DARKGREY, TFT_BLACK);
    lcd->setTextDatum(TL_DATUM);
    lcd->drawString("BRIGHTNESS", dimensions_.x, dimensions_.y + 2);
    Fonts::unload(lcd);

    isStaticDrawn_  = true;
    lastDrawnLevel_ = 0;  // force full segment redraw on next onDraw()
    clearDirty();
}

void BrightnessWidget::onDraw(bool forceRedraw) {
    if (!getLcd())
        return;

    const uint8_t current = displayManager_.getBrightness();
    if (!forceRedraw && current == lastDrawnLevel_) {
        clearDirty();
        return;
    }

    for (const auto& seg : segments_) {
        drawSegment(seg, seg.level == current);
    }

    lastDrawnLevel_   = current;
    lastUpdateTimeMs_ = millis();
    clearDirty();
}

void BrightnessWidget::drawSegment(const Segment& seg, bool active) {
    LGFX* lcd = getLcd();

    const uint16_t segY = dimensions_.y + kLabelH + kGap;
    const uint16_t segH = dimensions_.height - kLabelH - kGap;

    const uint16_t bgColor   = active ? seg.activeColor
                                      : static_cast<uint16_t>(0x2104);  // ~#202020
    const uint16_t textColor = active ? TFT_BLACK
                                      : static_cast<uint16_t>(0x6B4D);  // mid-grey

    lcd->fillRoundRect(seg.x, segY, seg.width, segH, kRadius, bgColor);

    Fonts::loadLabel(lcd);
    lcd->setTextColor(textColor, bgColor);
    lcd->setTextDatum(MC_DATUM);
    lcd->drawString(seg.label,
                    static_cast<int32_t>(seg.x + seg.width / 2),
                    static_cast<int32_t>(segY  + segH / 2));
    Fonts::unload(lcd);
}

bool BrightnessWidget::handleTouch(uint16_t x, uint16_t y) {
    if (!isInitialized_ || !getLcd())
        return false;

    const uint16_t segY = dimensions_.y + kLabelH + kGap;
    const uint16_t segH = dimensions_.height - kLabelH - kGap;

    for (const auto& seg : segments_) {
        if (x >= seg.x && x < static_cast<uint16_t>(seg.x + seg.width) &&
            y >= segY   && y < static_cast<uint16_t>(segY + segH)) {

            if (seg.level != displayManager_.getBrightness()) {
                displayManager_.setBrightness(seg.level);
                markDirty();
            }
            return true;
        }
    }

    return false;
}
