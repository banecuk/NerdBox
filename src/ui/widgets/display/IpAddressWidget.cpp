#include "IpAddressWidget.h"

#include <cstring>

#include "core/resources/FontRegistry.h"

IpAddressWidget::IpAddressWidget(const WidgetInterface::Dimensions& dims,
                                 NetworkManager& networkManager, uint16_t textColor,
                                 uint16_t bgColor)
    : Widget(dims, 5000),
      networkManager_(networkManager),
      textColor_(textColor),
      bgColor_(bgColor) {}

// ---------------------------------------------------------------------------
// Layout — called once from drawStatic(); measures actual font heights so
// valueY_ is correct regardless of which font is active.
// ---------------------------------------------------------------------------
void IpAddressWidget::computeLayout() {
    LGFX* lcd = getLcd();
    if (!lcd) return;

    Fonts::loadLabel(lcd);
    const uint16_t labelH = static_cast<uint16_t>(lcd->fontHeight());
    Fonts::unload(lcd);

    Fonts::loadValue(lcd);
    const uint16_t valH = static_cast<uint16_t>(lcd->fontHeight());
    Fonts::unload(lcd);

    const uint16_t pad = 2;
    valueY_ = dimensions_.y + labelH + pad
              + ((dimensions_.height - labelH - pad) - valH) / 2;

    layoutReady_ = true;
}

void IpAddressWidget::onDrawStatic() {
    LGFX* lcd = getLcd();
    lcd->fillRect(dimensions_.x, dimensions_.y, dimensions_.width, dimensions_.height, bgColor_);

    if (!layoutReady_)
        computeLayout();

    drawCaptionLabel("IP ADDRESS", bgColor_);
}

void IpAddressWidget::onDraw(bool forceRedraw) {
    if (!getLcd())
        return;

    const bool connected = networkManager_.isConnected();
    char currentIp[16] = {};
    if (connected) {
        networkManager_.getLocalIp(currentIp, sizeof(currentIp));
    }

    const bool changed = (connected != lastConnected_) ||
                         (strncmp(currentIp, lastIp_, sizeof(currentIp)) != 0);

    if (forceRedraw || changed) {
        renderContent(connected, currentIp);
        lastConnected_ = connected;
        strncpy(lastIp_, currentIp, sizeof(lastIp_));
        lastUpdateTimeMs_ = millis();
    }

    clearDirty();
}

void IpAddressWidget::renderContent(bool connected, const char* ip) {
    LGFX* lcd = getLcd();

    Fonts::loadValue(lcd);
    lcd->setTextDatum(TL_DATUM);

    if (connected) {
        lcd->setTextColor(textColor_, bgColor_);
        lcd->drawString(ip, dimensions_.x, valueY_);
    } else {
        lcd->setTextColor(TFT_DARKGREY, bgColor_);
        lcd->drawString("Not connected", dimensions_.x, valueY_);
    }

    Fonts::unload(lcd);
}

bool IpAddressWidget::handleTouch(uint16_t x, uint16_t y) {
    return false;
}
