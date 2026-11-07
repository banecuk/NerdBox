#pragma once

#include <string>

#include "network/NetworkManager.h"
#include "ui/core/Colors.h"
#include "ui/resources/FontRegistry.h"
#include "ui/widgets/base/Widget.h"
#include "utils/CachedString.h"

class IpAddressWidget : public Widget {
 public:
    IpAddressWidget(const WidgetInterface::Dimensions& dims, NetworkManager& networkManager,
                    uint16_t textColor = Colors::kInfo, uint16_t bgColor = TFT_BLACK);

    bool handleTouch(uint16_t x, uint16_t y) override;

 protected:
    void onDraw(bool forceRedraw) override;
    void onDrawStatic() override;

 private:
    NetworkManager& networkManager_;
    uint16_t textColor_;
    uint16_t bgColor_;
    bool lastConnected_ = false;
    CachedString<16> lastIp_;

    uint16_t valueY_ = 0;  // computed from font metrics in computeLayout()
    bool layoutReady_ = false;

    void computeLayout();
    void renderContent(bool connected, const char* ip);
};
