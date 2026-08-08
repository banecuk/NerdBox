#include "SettingsScreen.h"

#include "ui/core/Layout.h"
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
    static constexpr uint16_t kContentW = Layout::kScreenW - 2 * kGutter;  // = 456

    // ── Top bar ────────────────────────────────────────────────────────────
    // Brightness selector — spans the full width.
    widgetManager_.addWidget(std::unique_ptr<BrightnessWidget>(
        new BrightnessWidget(WidgetInterface::Dimensions{kGutter, 0, kContentW, kBrightnessH},
                             *uiController_->getDisplayManager())));

    // Dim at night — toggles dimming brightness 50% between 20:00 and 06:00.
    // Actual dimming is applied by DimAtNightJob/DisplayManager in the
    // background; this widget just reflects/writes the enabled flag. Sits
    // directly below the brightness selector, moving with its height.
    // Width matches two BrightnessWidget segments + the gap between them:
    // segW = (kContentW - kGap*5) / 6 = 73, so 73*2 + kGap(3) = 149.
    static constexpr uint16_t kSwitchW = 149;
    widgetManager_.addWidget(std::unique_ptr<SwitchWidget>(new SwitchWidget(
        WidgetInterface::Dimensions{kGutter, kSwitchY, kSwitchW, kSwitchH}, "DIM AT NIGHT",
        [this]() { return uiController_->getDisplayManager()->isDimAtNightEnabled(); },
        [this](bool enabled) {
            uiController_->getDisplayManager()->setDimAtNightEnabled(enabled);
        })));

    // Reset — same row as DIM AT NIGHT, right-aligned on the gutter. 48px
    // tall per the touch-target rule, vertically centered in the 64px row.
    static constexpr uint16_t kResetW = 150;
    static constexpr uint16_t kResetH = 48;
    static constexpr uint16_t kResetX = Layout::kScreenW - kGutter - kResetW;  // = 318
    static constexpr uint16_t kResetY = kSwitchY + (kSwitchH - kResetH) / 2;   // = 80

    widgetManager_.addWidget(std::unique_ptr<ButtonWidget>(new ButtonWidget(
        uiController_->getDisplayContext(), "Reset",
        WidgetInterface::Dimensions{kResetX, kResetY, kResetW, kResetH}, 0, EventType::RESET_DEVICE,
        [this](EventType action) { this->handleAction(action); }, TFT_RED, TFT_WHITE)));

    // ── Info widgets ────────────────────────────────────────────────────────
    // IP address / Uptime — sit below the switch/reset row.
    static constexpr uint16_t kInfoRowY = kSwitchY + kSwitchH + 16;  // = 152
    static constexpr uint16_t kIpWidth = 220;
    static constexpr uint16_t kUptimeX = kGutter + kIpWidth + kGutter;  // = 244

    widgetManager_.addWidget(std::unique_ptr<IpAddressWidget>(new IpAddressWidget(
        WidgetInterface::Dimensions{kGutter, kInfoRowY, kIpWidth, 40}, networkManager_)));

    widgetManager_.addWidget(std::unique_ptr<UptimeWidget>(new UptimeWidget(
        WidgetInterface::Dimensions{kUptimeX, kInfoRowY, 200, 40}, systemMetrics_)));

    // ── Bottom bar ─────────────────────────────────────────────────────────
    // Clock — bottom-right, right edge on the gutter. Row height bumped from
    // 24 to 32px so Mono24 glyphs get vertical padding instead of touching
    // the box edges.
    static constexpr uint16_t kClockH = 32;
    static constexpr uint16_t kClockX = Layout::kScreenW - kGutter - Layout::kClockW;  // = 318
    static constexpr uint16_t kClockY = 280;

    widgetManager_.addWidget(std::unique_ptr<ClockWidget>(
        new ClockWidget(WidgetInterface::Dimensions{kClockX, kClockY, Layout::kClockW, kClockH},
                        1000, TFT_YELLOW, TFT_BLACK)));

    // Back button — flush with the left screen edge (x=0), same shared
    // bottom band as MainScreen/GameScreen's bottom-left button (center 296),
    // matching the clock's center below.
    widgetManager_.addWidget(std::unique_ptr<ButtonWidget>(new ButtonWidget(
        uiController_->getDisplayContext(), "<",
        WidgetInterface::Dimensions{0, Layout::kBottomBarY, Layout::kButtonSize,
                                    Layout::kButtonSize},
        0, EventType::SHOW_MAIN, [this](EventType action) { this->handleAction(action); },
        TFT_BLACK, TFT_WHITE)));
}
