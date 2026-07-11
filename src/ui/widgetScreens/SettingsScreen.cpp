#include "SettingsScreen.h"

#include "ui/widgets/display/IpAddressWidget.h"

SettingsScreen::SettingsScreen(LoggerInterface& logger, UiController* uiController,
                               const AppSettings& config, NetworkManager& networkManager,
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

    // One 12px outer gutter on both sides for every widget on this screen.
    static constexpr uint16_t kGutter = 12;
    static constexpr uint16_t kScreenW = 480;
    static constexpr uint16_t kContentW = kScreenW - 2 * kGutter;  // = 456

    // ── Top bar ────────────────────────────────────────────────────────────
    // Brightness selector — now spans the full width since Reset moved down.
    widgetManager_.addWidget(std::unique_ptr<BrightnessWidget>(
        new BrightnessWidget(WidgetInterface::Dimensions{kGutter, 0, kContentW, kBrightnessH},
                             *uiController_->getDisplayManager())));

    // Dim at night — toggles dimming brightness 50% between 20:00 and 06:00.
    // Actual dimming is applied by DimAtNightJob/DisplayManager in the
    // background; this widget just reflects/writes the enabled flag. Sits
    // directly below the brightness selector, moving with its height.
    widgetManager_.addWidget(std::unique_ptr<SwitchWidget>(new SwitchWidget(
        WidgetInterface::Dimensions{kGutter, kSwitchY, 160, kSwitchH}, "DIM AT NIGHT",
        [this]() { return uiController_->getDisplayManager()->isDimAtNightEnabled(); },
        [this](bool enabled) {
            uiController_->getDisplayManager()->setDimAtNightEnabled(enabled);
        })));

    // ── Info widgets ────────────────────────────────────────────────────────
    // IP address / Uptime — sit below the switch, well clear of the Reset
    // button now parked just above the clock.
    static constexpr uint16_t kInfoRowY = kSwitchY + kSwitchH + 16;  // = 152
    static constexpr uint16_t kIpWidth = 220;
    static constexpr uint16_t kUptimeX = kGutter + kIpWidth + kGutter;  // = 244

    widgetManager_.addWidget(std::unique_ptr<IpAddressWidget>(new IpAddressWidget(
        WidgetInterface::Dimensions{kGutter, kInfoRowY, kIpWidth, 40}, networkManager_)));

    widgetManager_.addWidget(std::unique_ptr<UptimeWidget>(
        new UptimeWidget(WidgetInterface::Dimensions{kUptimeX, kInfoRowY, 200, 40}, systemMetrics_)));

    // ── Bottom bar ─────────────────────────────────────────────────────────
    // Clock — bottom-right, right edge on the gutter. Row height bumped from
    // 24 to 32px so Mono24 glyphs get vertical padding instead of touching
    // the box edges.
    static constexpr uint16_t kClockW = 150;
    static constexpr uint16_t kClockH = 32;
    static constexpr uint16_t kClockX = kScreenW - kGutter - kClockW;  // = 318
    static constexpr uint16_t kClockY = 280;

    widgetManager_.addWidget(std::unique_ptr<ClockWidget>(new ClockWidget(
        WidgetInterface::Dimensions{kClockX, kClockY, kClockW, kClockH}, 1000, TFT_YELLOW,
        TFT_BLACK)));

    // Reset — directly above the clock, same right gutter. 48px tall per the
    // touch-target rule (was 40) — it's the most dangerous action on screen.
    static constexpr uint16_t kResetH = 48;
    static constexpr uint16_t kResetY = kClockY - 8 - kResetH;  // = 224

    widgetManager_.addWidget(std::unique_ptr<ButtonWidget>(new ButtonWidget(
        uiController_->getDisplayContext(), "Reset",
        WidgetInterface::Dimensions{kClockX, kResetY, kClockW, kResetH}, 0,
        EventType::RESET_DEVICE, [this](EventType action) { this->handleAction(action); },
        TFT_RED, TFT_WHITE)));

    // Back button — bottom-left, same left gutter as everything else.
    widgetManager_.addWidget(std::unique_ptr<ButtonWidget>(new ButtonWidget(
        uiController_->getDisplayContext(), "<",
        WidgetInterface::Dimensions{kGutter, 320 - 1 - 48, 48, 48}, 0, EventType::SHOW_MAIN,
        [this](EventType action) { this->handleAction(action); }, TFT_BLACK, TFT_WHITE)));
}
