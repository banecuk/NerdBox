#include "SettingsScreen.h"

#include "ui/widgets/display/IpAddressWidget.h"

SettingsScreen::SettingsScreen(LoggerInterface& logger, UiController* uiController,
                               AppConfigInterface& config, NetworkManager& networkManager,
                               ApplicationMetrics& systemMetrics)
    : BaseWidgetScreen(logger, uiController, config),
      networkManager_(networkManager),
      systemMetrics_(systemMetrics) {}

void SettingsScreen::createWidgets() {
    // Row heights below are taller than a typical 48px bar (64px) so the
    // brightness segments and the dim-at-night track stay comfortable tap
    // targets — the previous 48/40px rows were too short to hit reliably.
    static constexpr uint16_t kBrightnessH = 64;
    static constexpr uint16_t kSwitchY = kBrightnessH + 8;  // = 72
    static constexpr uint16_t kSwitchH = 64;

    // ── Top bar ────────────────────────────────────────────────────────────
    // Brightness selector — now spans the full width since Reset moved down.
    widgetManager_.addWidget(std::unique_ptr<BrightnessWidget>(
        new BrightnessWidget(WidgetInterface::Dimensions{0, 0, 480, kBrightnessH},
                             *uiController_->getDisplayManager())));

    // Dim at night — toggles dimming brightness 50% between 20:00 and 06:00.
    // Actual dimming is applied by DimAtNightJob/DisplayManager in the
    // background; this widget just reflects/writes the enabled flag. Sits
    // directly below the brightness selector, moving with its height.
    widgetManager_.addWidget(std::unique_ptr<SwitchWidget>(new SwitchWidget(
        WidgetInterface::Dimensions{0, kSwitchY, 160, kSwitchH}, "DIM AT NIGHT",
        [this]() { return uiController_->getDisplayManager()->isDimAtNightEnabled(); },
        [this](bool enabled) {
            uiController_->getDisplayManager()->setDimAtNightEnabled(enabled);
        })));

    // ── Info widgets ────────────────────────────────────────────────────────
    // IP address / Uptime — sit below the switch, well clear of the Reset
    // button now parked just above the clock.
    static constexpr uint16_t kInfoRowY = kSwitchY + kSwitchH + 16;  // = 152

    widgetManager_.addWidget(std::unique_ptr<IpAddressWidget>(
        new IpAddressWidget(WidgetInterface::Dimensions{12, kInfoRowY, 220, 40}, networkManager_)));

    widgetManager_.addWidget(std::unique_ptr<UptimeWidget>(
        new UptimeWidget(WidgetInterface::Dimensions{260, kInfoRowY, 200, 40}, systemMetrics_)));

    // ── Bottom bar ─────────────────────────────────────────────────────────
    // Clock — bottom-right
    widgetManager_.addWidget(std::unique_ptr<ClockWidget>(new ClockWidget(
        WidgetInterface::Dimensions{328, 288, 150, 24}, 1000, TFT_YELLOW, TFT_BLACK)));

    // Reset — moved down, directly above the clock (was top-right).
    widgetManager_.addWidget(std::unique_ptr<ButtonWidget>(new ButtonWidget(
        uiController_->getDisplayContext(), "Reset",
        WidgetInterface::Dimensions{328, 288 - 8 - 40, 150, 40}, 0, EventType::RESET_DEVICE,
        [this](EventType action) { this->handleAction(action); }, TFT_RED, TFT_WHITE)));

    // Back button — bottom-left
    widgetManager_.addWidget(std::unique_ptr<ButtonWidget>(new ButtonWidget(
        uiController_->getDisplayContext(), "<",
        WidgetInterface::Dimensions{0, 320 - 1 - 48, 48, 48}, 0, EventType::SHOW_MAIN,
        [this](EventType action) { this->handleAction(action); }, TFT_BLACK, TFT_WHITE)));
}
