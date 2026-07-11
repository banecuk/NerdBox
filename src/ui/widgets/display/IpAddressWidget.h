#pragma once

#include <string>

#include "core/resources/FontRegistry.h"
#include "network/NetworkManager.h"
#include "ui/widgets/base/Widget.h"

class IpAddressWidget : public Widget {
 public:
    IpAddressWidget(const WidgetInterface::Dimensions& dims, NetworkManager& networkManager,
                    uint16_t textColor = TFT_CYAN, uint16_t bgColor = TFT_BLACK);

    bool handleTouch(uint16_t x, uint16_t y) override;

 protected:
    void onDraw(bool forceRedraw) override;
    void onDrawStatic() override;

 private:
    NetworkManager& networkManager_;
    uint16_t textColor_;
    uint16_t bgColor_;
    bool lastConnected_ = false;
    char lastIp_[16] = {};

    uint16_t valueY_ = 0;  // computed from font metrics in computeLayout()
    bool     layoutReady_ = false;

    void computeLayout();
    void renderContent(bool connected, const char* ip);
};
