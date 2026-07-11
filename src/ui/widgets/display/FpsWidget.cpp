#include "FpsWidget.h"

#include "core/resources/FontRegistry.h"

static constexpr uint16_t kBgColor = TFT_BLACK;
static constexpr uint16_t kLabelColor = TFT_DARKGREY;
static constexpr uint16_t kValueColor = TFT_GREEN;
static constexpr uint16_t kPlaceholderColor = 0x2104;  // very dark gray (RGB565)

FpsWidget::FpsWidget(DisplayContext& context, const WidgetInterface::Dimensions& dims,
                     uint32_t updateIntervalMs, PcMetrics& pcMetrics)
    : Widget(dims, updateIntervalMs),
      pcMetrics_(pcMetrics),
      freshnessGuard_(pcMetrics.is_available, pcMetrics.last_update_timestamp) {}

void FpsWidget::onDrawStatic() {
    clearArea();

    LGFX* lcd = getLcd();

    // "FPS" label
    Fonts::loadLabel(lcd);
    lcd->setTextColor(kLabelColor, kBgColor);
    lcd->setTextDatum(TC_DATUM);
    lcd->drawString("FPS", dimensions_.x + dimensions_.width / 2, dimensions_.y + 4);
    Fonts::unload(lcd);
}

void FpsWidget::onDraw(bool forceRedraw) {
    if (!getLcd())
        return;

    const int16_t fps = freshnessGuard_.isFresh() ? pcMetrics_.gpu_fps : int16_t(-1);
    const bool hasValue = (fps != -1);

    if (hasValue != lastVisible_) {
        if (hasValue) {
            // Number just appeared — render it
            renderFps(fps);
        } else {
            // Number just disappeared — show placeholder, keep the label
            renderPlaceholder();
        }
        lastVisible_ = hasValue;
        lastDrawnFps_ = fps;
        lastUpdateTimeMs_ = millis();
        clearDirty();
        return;
    }

    if (!hasValue) {
        if (forceRedraw) {
            renderPlaceholder();
        }
        clearDirty();
        return;
    }

    // Value was and still is present — repaint only when it changes
    if (forceRedraw || fps != lastDrawnFps_) {
        renderFps(fps);
        lastDrawnFps_ = fps;
        lastUpdateTimeMs_ = millis();
    }

    clearDirty();
}

void FpsWidget::renderFps(int16_t fps) {
    LGFX* lcd = getLcd();

    clearValueArea();

    char buf[6];
    snprintf(buf, sizeof(buf), "%d", static_cast<int>(fps));

    const uint16_t valueAreaY = dimensions_.y + 16;
    const uint16_t valueAreaH = dimensions_.height - 16;

    // NotoSansMono24 fills the 56 px value area well and keeps digits
    // fixed-width so the number doesn't shift left/right as it changes.
    Fonts::loadMono(lcd);
    lcd->setTextColor(kValueColor, kBgColor);
    lcd->setTextDatum(MC_DATUM);
    lcd->drawString(buf, dimensions_.x + dimensions_.width / 2, valueAreaY + valueAreaH / 2);
    Fonts::unload(lcd);
}

void FpsWidget::renderPlaceholder() {
    LGFX* lcd = getLcd();
    if (!lcd)
        return;

    clearValueArea();

    const uint16_t valueAreaY = dimensions_.y + 16;
    const uint16_t valueAreaH = dimensions_.height - 16;

    Fonts::loadMono(lcd);
    lcd->setTextColor(kPlaceholderColor, kBgColor);
    lcd->setTextDatum(MC_DATUM);
    lcd->drawString("---", dimensions_.x + dimensions_.width / 2, valueAreaY + valueAreaH / 2);
    Fonts::unload(lcd);
}

void FpsWidget::clearValueArea() {
    if (!getLcd())
        return;
    const uint16_t valueAreaY = dimensions_.y + 16;
    getLcd()->fillRect(dimensions_.x, valueAreaY, dimensions_.width, dimensions_.height - 16,
                       kBgColor);
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
