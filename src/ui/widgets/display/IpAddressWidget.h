#pragma once

#include <string>

#include "network/NetworkManager.h"
#include "ui/widgets/base/Widget.h"

// Displays the device's WiFi IP address on the settings screen.
// Shows "Not connected" when WiFi is down, the IP address when connected.
class IpAddressWidget : public Widget {
 public:
    IpAddressWidget(const WidgetInterface::Dimensions& dims, NetworkManager& networkManager,
                    uint16_t textColor = TFT_CYAN, uint16_t bgColor = TFT_BLACK);

    void drawStatic() override;
    bool handleTouch(uint16_t x, uint16_t y) override;

 protected:
    void onDraw(bool forceRedraw) override;

 private:
    NetworkManager& networkManager_;
    uint16_t textColor_;
    uint16_t bgColor_;
    bool lastConnected_ = false;
    char lastIp_[16] = {};  // max "255.255.255.255" + null

    void renderContent(bool connected, const char* ip);
};
