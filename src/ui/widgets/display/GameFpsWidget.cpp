#include "GameFpsWidget.h"

#include "core/resources/FontRegistry.h"
#include "ui/core/Colors.h"

static constexpr uint16_t kBgColor = TFT_BLACK;
static constexpr uint16_t kValueColor = TFT_GREEN;
static constexpr uint16_t kPlaceholderColor = Colors::kHairline;
static constexpr uint16_t kScaleLabelColor = TFT_DARKGREY;
static constexpr uint16_t kBarColor = TFT_DARKGREEN;
static constexpr uint16_t kNewestBarColor = TFT_GREEN;

GameFpsWidget::GameFpsWidget(DisplayContext& context, const WidgetInterface::Dimensions& dims,
                             uint32_t updateIntervalMs, PcMetrics& pcMetrics)
    : Widget(dims, updateIntervalMs), pcMetrics_(pcMetrics), freshnessGuard_(pcMetrics.freshness) {}

void GameFpsWidget::computePlotLayout() {
    plotX_ = dimensions_.x + kNumberBlockWidth + kPlotLeftGap;
    plotY_ = dimensions_.y + kPlotMarginTop;
    plotWidth_ = kHistorySize * kColWidth;
    plotHeight_ = dimensions_.height - kPlotMarginTop - kPlotMarginBottom;
    // Clamp so a narrower-than-designed dims never overflows past the widget.
    const uint16_t maxWidth =
        dimensions_.width - kNumberBlockWidth - kPlotLeftGap - kPlotRightMargin;
    if (plotWidth_ > maxWidth)
        plotWidth_ = maxWidth;
}

void GameFpsWidget::onDrawStatic() {
    LGFX* lcd = getLcd();
    lcd->fillRect(dimensions_.x, dimensions_.y, dimensions_.width, dimensions_.height, kBgColor);

    computePlotLayout();

    // "FPS" caption, top-left of the number block.
    Fonts::loadLabel(lcd);
    lcd->setTextColor(TFT_DARKGREY, kBgColor);
    lcd->setTextDatum(TL_DATUM);
    lcd->drawString("FPS", dimensions_.x + 4, dimensions_.y + 2);
    Fonts::unload(lcd);

    // Plot border.
    lcd->drawRect(plotX_ - 1, plotY_ - 1, plotWidth_ + 2, plotHeight_ + 2, Colors::kHairline);

    for (auto& h : lastColHeight_)
        h = 0;
    scaleChanged_ = true;
    drawScaleLabels();
}

void GameFpsWidget::drawScaleLabels() {
    if (!scaleChanged_)
        return;
    LGFX* lcd = getLcd();

    // Clear the label gutter to the left margin of the plot before redrawing.
    lcd->fillRect(dimensions_.x + kNumberBlockWidth, plotY_, kPlotLeftGap - 1, plotHeight_,
                  kBgColor);

    char buf[8];
    Fonts::loadLabel(lcd);
    lcd->setTextColor(kScaleLabelColor, kBgColor);

    lcd->setTextDatum(TR_DATUM);
    snprintf(buf, sizeof(buf), "%d", static_cast<int>(yMax_));
    lcd->drawString(buf, plotX_ - 3, plotY_ - 1);

    lcd->setTextDatum(BR_DATUM);
    lcd->drawString("0", plotX_ - 3, plotY_ + plotHeight_ + 1);

    Fonts::unload(lcd);
    scaleChanged_ = false;
}

void GameFpsWidget::updateScale(int16_t sampleValue) {
    if (sampleValue < 0)
        return;

    if (sampleValue > yMax_) {
        while (sampleValue > yMax_)
            yMax_ += kScaleStep;
        scaleChanged_ = true;
        return;
    }

    // Shrink only when the whole retained history comfortably fits a smaller
    // scale — avoids the axis flapping on every dip.
    if (yMax_ > kMinScale) {
        int16_t bufMax = 0;
        for (size_t i = 0; i < history_.size(); ++i) {
            const int16_t v = history_.at(i);
            if (v > bufMax)
                bufMax = v;
        }
        if (bufMax < (yMax_ * 6) / 10) {
            yMax_ -= kScaleStep;
            if (yMax_ < kMinScale)
                yMax_ = kMinScale;
            scaleChanged_ = true;
        }
    }
}

void GameFpsWidget::sampleIfNeeded() {
    if (!freshnessGuard_.isFresh())
        return;
    if (pcMetrics_.freshness.lastUpdateMs() == lastSampledTimestamp_)
        return;

    lastSampledTimestamp_ = pcMetrics_.freshness.lastUpdateMs();
    const int16_t fps = pcMetrics_.gpu_fps;
    history_.push(fps);
    updateScale(fps);
}

void GameFpsWidget::drawSparkline(bool forceFullRepaint) {
    LGFX* lcd = getLcd();
    const size_t count = history_.size();
    const size_t offset = kHistorySize - count;  // columns [0, offset) are still blank

    lcd->startWrite();
    for (size_t col = 0; col < kHistorySize; ++col) {
        int16_t sample = kNoSample;
        if (col >= offset) {
            sample = history_.at(col - offset);
        }

        uint8_t newHeight = 0;
        if (sample >= 0 && yMax_ > 0) {
            uint32_t h = (static_cast<uint32_t>(sample) * (plotHeight_ - 1)) / yMax_;
            if (h > plotHeight_ - 1)
                h = plotHeight_ - 1;
            newHeight = static_cast<uint8_t>(h + 1);
        }

        const uint8_t oldHeight = lastColHeight_[col];
        if (!forceFullRepaint && newHeight == oldHeight)
            continue;

        const uint16_t x = plotX_ + static_cast<uint16_t>(col) * kColWidth;
        const uint16_t w = kColWidth - 1;
        const bool isNewest = (count > 0 && col == kHistorySize - 1);
        const uint16_t color = isNewest ? kNewestBarColor : kBarColor;

        if (newHeight > oldHeight) {
            const uint16_t delta = newHeight - oldHeight;
            if (newHeight > 0)
                lcd->fillRect(x, plotY_ + plotHeight_ - newHeight, w, delta, color);
        } else if (newHeight < oldHeight) {
            const uint16_t delta = oldHeight - newHeight;
            lcd->fillRect(x, plotY_ + plotHeight_ - oldHeight, w, delta, kBgColor);
            if (newHeight > 0 && forceFullRepaint)
                lcd->fillRect(x, plotY_ + plotHeight_ - newHeight, w, newHeight, color);
        } else if (forceFullRepaint && newHeight > 0) {
            lcd->fillRect(x, plotY_ + plotHeight_ - newHeight, w, newHeight, color);
        }

        lastColHeight_[col] = newHeight;
    }
    lcd->endWrite();
}

void GameFpsWidget::onDraw(bool forceRedraw) {
    if (!getLcd())
        return;

    sampleIfNeeded();

    const int16_t fps = freshnessGuard_.isFresh() ? pcMetrics_.gpu_fps : int16_t(-1);
    const bool hasValue = (fps != -1);

    if (hasValue != lastVisible_) {
        if (hasValue) {
            renderFpsNumber(fps);
        } else {
            renderPlaceholder();
        }
        lastVisible_ = hasValue;
        lastDrawnFps_ = fps;
    } else if (hasValue && (forceRedraw || fps != lastDrawnFps_)) {
        renderFpsNumber(fps);
        lastDrawnFps_ = fps;
    } else if (!hasValue && forceRedraw) {
        renderPlaceholder();
    }

    const bool scaleJustChanged = scaleChanged_;
    if (scaleJustChanged)
        drawScaleLabels();
    drawSparkline(forceRedraw || scaleJustChanged);

    lastUpdateTimeMs_ = millis();
    clearDirty();
}

void GameFpsWidget::clearNumberValueArea() {
    LGFX* lcd = getLcd();
    const uint16_t valueAreaY = dimensions_.y + 20;
    lcd->fillRect(dimensions_.x, valueAreaY, kNumberBlockWidth, dimensions_.height - 20, kBgColor);
}

void GameFpsWidget::renderFpsNumber(int16_t fps) {
    LGFX* lcd = getLcd();
    clearNumberValueArea();

    char buf[8];
    snprintf(buf, sizeof(buf), "%d", static_cast<int>(fps));

    const uint16_t cx = dimensions_.x + kNumberBlockWidth / 2;
    const uint16_t cy = dimensions_.y + 20 + (dimensions_.height - 20) / 2;

    Fonts::loadMono(lcd);
    lcd->setTextSize(2);
    lcd->setTextColor(kValueColor, kBgColor);
    lcd->setTextDatum(MC_DATUM);
    lcd->drawString(buf, cx, cy);
    lcd->setTextSize(1);  // restore — setTextSize persists past unloadFont()
    Fonts::unload(lcd);
}

void GameFpsWidget::renderPlaceholder() {
    LGFX* lcd = getLcd();
    if (!lcd)
        return;
    clearNumberValueArea();

    const uint16_t cx = dimensions_.x + kNumberBlockWidth / 2;
    const uint16_t cy = dimensions_.y + 20 + (dimensions_.height - 20) / 2;

    Fonts::loadMono(lcd);
    lcd->setTextSize(2);
    lcd->setTextColor(kPlaceholderColor, kBgColor);
    lcd->setTextDatum(MC_DATUM);
    lcd->drawString("---", cx, cy);
    lcd->setTextSize(1);
    Fonts::unload(lcd);
}

bool GameFpsWidget::handleTouch(uint16_t x, uint16_t y) {
    return false;
}
