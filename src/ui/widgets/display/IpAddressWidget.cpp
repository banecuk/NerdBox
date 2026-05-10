#include "IpAddressWidget.h"

#include <cstring>

IpAddressWidget::IpAddressWidget(const WidgetInterface::Dimensions& dims,
                                 NetworkManager& networkManager, uint16_t textColor,
                                 uint16_t bgColor)
    : Widget(dims, 5000),  // re-check every 5 s — IP won't change faster than that
      networkManager_(networkManager),
      textColor_(textColor),
      bgColor_(bgColor) {}

void IpAddressWidget::drawStatic() {
    if (!isInitialized_ || !getLcd())
        return;

    LGFX* lcd = getLcd();
    lcd->fillRect(dimensions_.x, dimensions_.y, dimensions_.width, dimensions_.height, bgColor_);

    // "IP ADDRESS" label
    lcd->setTextSize(1);
    lcd->setTextColor(TFT_DARKGREY, bgColor_);
    lcd->setTextDatum(TL_DATUM);
    lcd->drawString("IP ADDRESS", dimensions_.x, dimensions_.y + 2);

    isStaticDrawn_ = true;
    clearDirty();
}

void IpAddressWidget::onDraw(bool forceRedraw) {
    if (!getLcd())
        return;

    const bool connected = networkManager_.isConnected();
    char currentIp[16] = {};
    if (connected) {
        String ip = networkManager_.getLocalIp();
        strncpy(currentIp, ip.c_str(), sizeof(currentIp) - 1);
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

    // Clear value area (below the label)
    const uint16_t valueY = dimensions_.y + 14;
    const uint16_t valueH = dimensions_.height - 14;
    lcd->fillRect(dimensions_.x, valueY, dimensions_.width, valueH, bgColor_);

    lcd->setTextSize(2);
    lcd->setTextDatum(TL_DATUM);

    if (connected) {
        lcd->setTextColor(textColor_, bgColor_);
        lcd->drawString(ip, dimensions_.x, valueY + 2);
    } else {
        lcd->setTextColor(TFT_DARKGREY, bgColor_);
        lcd->drawString("Not connected", dimensions_.x, valueY + 2);
    }
}

bool IpAddressWidget::handleTouch(uint16_t x, uint16_t y) {
    return false;
}
