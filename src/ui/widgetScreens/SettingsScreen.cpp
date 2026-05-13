#include "SettingsScreen.h"

#include "ui/widgets/display/IpAddressWidget.h"

SettingsScreen::SettingsScreen(LoggerInterface& logger, UiController* uiController,
                               AppConfigInterface& config, NetworkManager& networkManager,
                               ApplicationMetrics& systemMetrics)
    : BaseWidgetScreen(logger, uiController, config),
      networkManager_(networkManager),
      systemMetrics_(systemMetrics) {}

void SettingsScreen::createWidgets() {
    // ── Top bar ────────────────────────────────────────────────────────────
    // Brightness selector — spans from left edge to just before Reset button.
    // Three tappable level segments with a visible active-level indicator.
    widgetManager_.addWidget(std::unique_ptr<BrightnessWidget>(new BrightnessWidget(
        WidgetInterface::Dimensions{0, 0, 480 - 100 - 4, 48},
        *uiController_->getDisplayManager())));

    // Reset — top-right
    widgetManager_.addWidget(std::unique_ptr<ButtonWidget>(new ButtonWidget(
        uiController_->getDisplayContext(), "Reset",
        WidgetInterface::Dimensions{480 - 100, 0, 100, 48}, 0, EventType::RESET_DEVICE,
        [this](EventType action) { this->handleAction(action); }, TFT_RED, TFT_WHITE)));

    // ── Info widgets ────────────────────────────────────────────────────────
    // IP address — left column, y=100
    widgetManager_.addWidget(std::unique_ptr<IpAddressWidget>(new IpAddressWidget(
        WidgetInterface::Dimensions{12, 100, 220, 40}, networkManager_)));

    // Uptime — right of IP, same row
    widgetManager_.addWidget(std::unique_ptr<UptimeWidget>(new UptimeWidget(
        WidgetInterface::Dimensions{260, 100, 200, 40}, systemMetrics_)));

    // ── Bottom bar ─────────────────────────────────────────────────────────
    // Clock — bottom-right
    widgetManager_.addWidget(std::unique_ptr<ClockWidget>(new ClockWidget(
        WidgetInterface::Dimensions{328, 288, 150, 24}, 1000, TFT_YELLOW, TFT_BLACK, 3)));

    // Back button — bottom-left
    widgetManager_.addWidget(std::unique_ptr<ButtonWidget>(new ButtonWidget(
        uiController_->getDisplayContext(), "<",
        WidgetInterface::Dimensions{0, 320 - 1 - 48, 48, 48}, 0, EventType::SHOW_MAIN,
        [this](EventType action) { this->handleAction(action); }, TFT_BLACK, TFT_WHITE)));
}
