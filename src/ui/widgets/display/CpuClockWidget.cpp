#include "CpuClockWidget.h"

#include <algorithm>
#include <cstdio>

#include "ui/core/Colors.h"
#include "ui/core/UiText.h"
#include "ui/resources/FontRegistry.h"

namespace {
constexpr uint16_t kBgColor = TFT_BLACK;
constexpr uint16_t kLabelColor = TFT_DARKGREY;

constexpr float kClockColorMinMHz = 1000.0f;
constexpr float kClockColorMaxMHz = 5000.0f;

// Maps an MHz value onto the 0-99 index Colors::getColorFromPercentGrayGreen()
// looks up in its precomputed gradient table — the light-gray-to-light-green
// blend itself is computed once at startup (Colors::generateGradient()), not
// re-blended on every draw.
uint8_t clockPercent(float mhz) {
    const float clamped =
        mhz < kClockColorMinMHz ? kClockColorMinMHz : (mhz > kClockColorMaxMHz ? kClockColorMaxMHz : mhz);
    const float t = (clamped - kClockColorMinMHz) / (kClockColorMaxMHz - kClockColorMinMHz);
    return static_cast<uint8_t>(t * 99.0f + 0.5f);
}
}  // namespace

CpuClockWidget::CpuClockWidget(DisplayContext& context, const WidgetInterface::Dimensions& dims,
                               uint32_t updateIntervalMs, const CpuClockData& data)
    : Widget(dims, updateIntervalMs), data_(data), freshnessGuard_(data.freshness) {}

bool CpuClockWidget::ensureLayoutInitialized() {
    if (coreCount_ != 0) {
        return true;
    }

    const uint8_t detected = data_.coreCount;
    if (detected == 0) {
        return false;  // No CoreClocksMHz payload has arrived yet
    }

    coreCount_ = detected;
    gridRows_ = (coreCount_ + kColumns - 1) / kColumns;
    cellWidth_ = dimensions_.width / kColumns;
    const uint16_t gridHeight = dimensions_.height - kBusLineHeight;
    cellHeight_ = gridRows_ > 0 ? gridHeight / gridRows_ : gridHeight;
    previousClockMHz_.assign(coreCount_, -1.0f);
    return true;
}

void CpuClockWidget::onDrawStatic() {
    getLcd()->fillRect(dimensions_.x, dimensions_.y, dimensions_.width, dimensions_.height,
                       kBgColor);
    std::fill(previousClockMHz_.begin(), previousClockMHz_.end(), -1.0f);
    previousBusSpeedMHz_ = -1.0f;
}

void CpuClockWidget::onDraw(bool forceRedraw) {
    if (!getLcd())
        return;

    const bool fresh = freshnessGuard_.isFresh() && ensureLayoutInitialized();

    if (!wasFresh_ && fresh) {
        onDrawStatic();
    }

    if (fresh) {
        drawGrid(forceRedraw || !wasFresh_);
        drawBusSpeed(forceRedraw || !wasFresh_);
    } else if (wasFresh_ || forceRedraw) {
        drawNoDataMessage();
    }

    wasFresh_ = fresh;
    lastUpdateTimeMs_ = millis();
    clearDirty();
}

void CpuClockWidget::drawGrid(bool forceRedraw) {
    // drawCell() itself swaps between loadLabel (core number) and loadMetric
    // (clock value) per cell, so no batch font load here — see its comment.
    for (uint8_t i = 0; i < coreCount_; ++i) {
        const float mhz = data_.coreClockMHz[i];
        if (forceRedraw || mhz != previousClockMHz_[i]) {
            drawCell(i, mhz);
            previousClockMHz_[i] = mhz;
        }
    }
}

void CpuClockWidget::drawCell(uint8_t index, float mhz) {
    LGFX* lcd = getLcd();
    const uint16_t col = index % kColumns;
    const uint16_t row = index / kColumns;
    const uint16_t x = dimensions_.x + col * cellWidth_;
    const uint16_t y = dimensions_.y + row * cellHeight_;

    lcd->fillRect(x, y, cellWidth_, cellHeight_, kBgColor);

    // Core number — dark grey, small label font, top-left corner. 1-indexed,
    // always 2 digits ("01", "02", ... "10", "11", ...) per the fixed layout.
    char numBuf[4];
    snprintf(numBuf, sizeof(numBuf), "%02u", static_cast<unsigned>(index) + 1);
    Fonts::loadLabel(lcd);
    lcd->setTextColor(TFT_DARKGREY, kBgColor);
    lcd->setTextDatum(TL_DATUM);
    lcd->drawString(numBuf, x + 4, y + 2);
    Fonts::unload(lcd);

    // Clock value — the cell's primary content: larger metric font, colored
    // by the gray-to-green gradient, centered in the remaining space below
    // the core-number label.
    char valBuf[8];
    snprintf(valBuf, sizeof(valBuf), "%d", static_cast<int>(mhz + 0.5f));
    Fonts::loadMetric(lcd);
    lcd->setTextColor(getContext().getColors().getColorFromPercentGrayGreen(clockPercent(mhz)),
                      kBgColor);
    lcd->setTextDatum(MC_DATUM);
    lcd->drawString(valBuf, x + cellWidth_ / 2, y + cellHeight_ * 2 / 3);
    Fonts::unload(lcd);
}

void CpuClockWidget::drawBusSpeed(bool forceRedraw) {
    const float bus = data_.busSpeedMHz;
    if (!forceRedraw && bus == previousBusSpeedMHz_) {
        return;
    }
    previousBusSpeedMHz_ = bus;

    LGFX* lcd = getLcd();
    const uint16_t y = dimensions_.y + dimensions_.height - kBusLineHeight;
    lcd->fillRect(dimensions_.x, y, dimensions_.width, kBusLineHeight, kBgColor);

    char buf[32];
    snprintf(buf, sizeof(buf), "Bus: %d MHz", static_cast<int>(bus + 0.5f));

    Fonts::loadLabel(lcd);
    lcd->setTextColor(kLabelColor, kBgColor);
    lcd->setTextDatum(MC_DATUM);
    lcd->drawString(buf, dimensions_.x + dimensions_.width / 2, y + kBusLineHeight / 2);
    Fonts::unload(lcd);
}

void CpuClockWidget::drawNoDataMessage() {
    LGFX* lcd = getLcd();
    lcd->fillRect(dimensions_.x, dimensions_.y, dimensions_.width, dimensions_.height, kBgColor);

    Fonts::loadLabel(lcd);
    lcd->setTextColor(TFT_DARKGREY, kBgColor);
    lcd->setTextDatum(MC_DATUM);
    lcd->drawString(UiText::kNoData, dimensions_.x + dimensions_.width / 2,
                    dimensions_.y + dimensions_.height / 2);
    Fonts::unload(lcd);
}
