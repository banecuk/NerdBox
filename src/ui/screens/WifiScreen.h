#pragma once

#include "config/AppSettings.h"
#include "network/NetworkManager.h"
#include "services/network/NetworkStatus.h"
#include "services/wifiScan/WifiScanData.h"
#include "ui/screens/base/BaseWidgetScreen.h"

// WiFi diagnostics screen: this device's own link (SSID/RSSI/IP/etc, via
// WifiLinkWidget), the internet-reachability probe state (NetworkWidget,
// reused unchanged/untappable here), and a live scan of nearby SSIDs
// (WifiScanListWidget). Entered by tapping NetworkWidget on MainScreen. See
// docs-local/13-wifi-screen-plan.md.
class WifiScreen : public BaseWidgetScreen {
 public:
    WifiScreen(LoggerInterface& logger, NetworkManager& networkManager, WifiScanData& wifiScan,
              const NetworkStatus& netStatus, UiController* uiController,
              const AppSettings& config);
    ~WifiScreen() override = default;

 private:
    void createWidgets() override;

    NetworkManager& networkManager_;
    WifiScanData& wifiScan_;
    const NetworkStatus& netStatus_;
};
