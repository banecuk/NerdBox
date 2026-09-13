#include "WifiScreen.h"

#include "ui/core/Layout.h"
#include "ui/core/Theme.h"
#include "ui/widgets/display/NetworkWidget.h"
#include "ui/widgets/display/WifiLinkWidget.h"
#include "ui/widgets/display/WifiScanListWidget.h"
#include "ui/widgets/interactive/ButtonWidget.h"

namespace {
// WifiLinkWidget's own internal split is 76px (SSID/bars/trace) + 42px
// (details strip) + a 1px hairline it draws itself — see
// docs-local/13-wifi-screen-plan.md §4.1/§4.2.
constexpr uint16_t kLinkH = 119;
constexpr uint16_t kListY = kLinkH + 1;

// Footer — same shape as CpuClockScreen's (back + one extra action button +
// NetworkWidget/clock), see §4.1's layout table.
constexpr uint16_t kRescanX = Layout::kButtonSize;
constexpr uint16_t kRescanW = 100;
constexpr uint16_t kNetWidgetX = kRescanX + kRescanW + 14;
constexpr uint16_t kNetWidgetW = 148;
constexpr uint16_t kNetWidgetH = 24;
constexpr uint16_t kNetWidgetY = Layout::kBottomBarY + (Layout::kBottomBarH - kNetWidgetH) / 2;
}  // namespace

WifiScreen::WifiScreen(LoggerInterface& logger, NetworkManager& networkManager,
                       WifiScanData& wifiScan, const NetworkStatus& netStatus,
                       UiController* uiController, const AppSettings& config)
    : BaseWidgetScreen(logger, uiController, config),
      networkManager_(networkManager),
      wifiScan_(wifiScan),
      netStatus_(netStatus) {}

void WifiScreen::createWidgets() {
    widgetManager_.addWidget(
        std::make_unique<WifiLinkWidget>(
            uiController_->getDisplayContext(),
            WidgetInterface::Dimensions{0, 0, Layout::kScreenW, kLinkH}, 1000, networkManager_,
            wifiScan_, config_),
        "wifi_link");

    widgetManager_.addWidget(
        std::make_unique<WifiScanListWidget>(
            WidgetInterface::Dimensions{0, kListY, Layout::kScreenW, Layout::kContentH - kListY},
            500, wifiScan_),
        "wifi_scan_list");

    addBackButton();

    // Rescan — a plain local action, not an EventBus round-trip: the screen
    // already holds the WifiScanData& it needs to set, and this action isn't
    // cross-cutting (see docs-local/13-wifi-screen-plan.md §4.4 option 1).
    widgetManager_.addWidget(
        std::make_unique<ButtonWidget>(
            uiController_->getDisplayContext(), "Rescan",
            WidgetInterface::Dimensions{kRescanX, Layout::kBottomBarY, kRescanW,
                                        Layout::kButtonSize},
            0, EventType::NONE,
            [this](EventType) { wifiScan_.rescanRequested.store(true); }, Theme::kSurface,
            TFT_WHITE),
        "rescan_button");

    // Internet-reachability probe state — untappable here (this screen *is*
    // the connectivity detail view already).
    widgetManager_.addWidget(
        std::make_unique<NetworkWidget>(
            WidgetInterface::Dimensions{kNetWidgetX, kNetWidgetY, kNetWidgetW, kNetWidgetH}, 1000,
            netStatus_),
        "network_status");

    addBottomClock();
}
