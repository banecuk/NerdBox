#include "FpsWidget.h"

// Text layout constants
static constexpr uint8_t kLabelTextSize = 1;  // small "FPS" label
static constexpr uint8_t kValueTextSize = 4;  // large FPS number
static constexpr uint16_t kBgColor = TFT_BLACK;
static constexpr uint16_t kLabelColor = TFT_DARKGREY;
static constexpr uint16_t kValueColor = TFT_GREEN;

FpsWidget::FpsWidget(DisplayContext& context, const WidgetInterface::Dimensions& dims,
                     uint32_t updateIntervalMs, PcMetrics& pcMetrics)
    : Widget(dims, updateIntervalMs), context_(context), pcMetrics_(pcMetrics) {}

void FpsWidget::drawStatic() {
    if (!isInitialized_ || !getLcd())
        return;

    clearArea();

    LGFX* lcd = getLcd();

    // "FPS" label at the top of the square
    lcd->setTextSize(kLabelTextSize);
    lcd->setTextColor(kLabelColor, kBgColor);
    lcd->setTextDatum(TC_DATUM);
    lcd->drawString("FPS", dimensions_.x + dimensions_.width / 2, dimensions_.y + 4);

    isStaticDrawn_ = true;
    clearDirty();
}

void FpsWidget::onDraw(bool forceRedraw) {
    if (!getLcd())
        return;

    const bool dataReady = pcMetrics_.is_available;
    const int16_t fps = pcMetrics_.gpu_fps;

    // The widget is only meaningful when data is live and a fullscreen FPS is reported
    const bool shouldShow = dataReady && (fps != -1);

    if (shouldShow != lastVisible_) {
        if (shouldShow) {
            // Transitioning to visible: draw static chrome and current value
            drawStatic();
            renderFps(fps);
        } else {
            // Transitioning to hidden: wipe the area clean
            clearArea();
        }
        lastVisible_ = shouldShow;
        lastDrawnFps_ = fps;
        lastUpdateTimeMs_ = millis();
        clearDirty();
        return;
    }

    if (!shouldShow) {
        clearDirty();
        return;
    }

    // Visible and was already visible — only repaint the number when it changes
    if (forceRedraw || fps != lastDrawnFps_) {
        renderFps(fps);
        lastDrawnFps_ = fps;
        lastUpdateTimeMs_ = millis();
    }

    clearDirty();
}

void FpsWidget::renderFps(int16_t fps) {
    LGFX* lcd = getLcd();

    // Clear the value area (below the "FPS" label)
    const uint16_t valueAreaY = dimensions_.y + 16;
    const uint16_t valueAreaH = dimensions_.height - 16;
    lcd->fillRect(dimensions_.x, valueAreaY, dimensions_.width, valueAreaH, kBgColor);

    char buf[6];
    snprintf(buf, sizeof(buf), "%d", static_cast<int>(fps));

    lcd->setTextSize(kValueTextSize);
    lcd->setTextColor(kValueColor, kBgColor);
    lcd->setTextDatum(MC_DATUM);
    lcd->drawString(buf, dimensions_.x + dimensions_.width / 2, valueAreaY + valueAreaH / 2);
}

void FpsWidget::clearArea() {
    if (!getLcd())
        return;
    getLcd()->fillRect(dimensions_.x, dimensions_.y, dimensions_.width, dimensions_.height,
                       kBgColor);
}

bool FpsWidget::handleTouch(uint16_t x, uint16_t y) {
    return false;
}
