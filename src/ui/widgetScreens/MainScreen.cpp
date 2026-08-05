#include "MainScreen.h"

MainScreen::MainScreen(LoggerInterface& logger, PcMetrics& pcMetrics, UiController* uiController,
                       const AppSettings& config, ApplicationMetrics& systemMetrics,
                       const AirQualityData& airQualityData, const NetworkStatus& netStatus)
    : BaseWidgetScreen(logger, uiController, config),
      pcMetrics_(pcMetrics),
      systemMetrics_(systemMetrics),
      airQualityData_(airQualityData),
      netStatus_(netStatus) {}

void MainScreen::createWidgets() {
    // Threads — full-width top row (FPS moved down beside MultiWidget)
    auto threadsWidget = std::unique_ptr<ThreadsWidget>(new ThreadsWidget(
        uiController_->getDisplayContext(), WidgetInterface::Dimensions{0, 0, 480, 60},
        config_.hardwareMonitorThreadsRefreshMs, pcMetrics_, config_, systemMetrics_));
    widgetManager_.addWidget(std::move(threadsWidget));

    // Game metrics grid — replaces PcMetricsWidget, directly below threads.
    // 78px tall → 26px tile rows (1px more top and bottom per tile vs
    // GameScreen's 90px → 30px) via GameMetricsWidget::rowHeight() rescaling.
    auto gameMetricsWidget = std::unique_ptr<GameMetricsWidget>(
        new GameMetricsWidget(uiController_->getDisplayContext(),
                              WidgetInterface::Dimensions{0, 60, 480, 78}, 100, pcMetrics_));
    gameMetricsWidget->setStaleTimeout(5000);
    widgetManager_.addWidget(std::move(gameMetricsWidget));

    // Disk band — slim strip, tappable to the disk screen. 27px tall: 4px
    // read/write activity lines + a ~19px borderless per-drive tile area that
    // fits the NotoSans15 value font and runs flush against both lines.
    widgetManager_.addWidget(std::unique_ptr<DiskBandWidget>(new DiskBandWidget(
        uiController_->getDisplayContext(), WidgetInterface::Dimensions{0, 138, 480, 27}, 100,
        pcMetrics_, EventType::SHOW_DISKS,
        [this](EventType action) { this->handleAction(action); })));

    // Air quality bar
    widgetManager_.addWidget(std::unique_ptr<AirQualityWidget>(
        new AirQualityWidget(WidgetInterface::Dimensions{0, 165, 480, 44}, 5000, airQualityData_)));

    // Multifunctional widget — left of the FPS display; width trimmed from the
    // right to make room for a minimal-width FPS tile beside it.
    widgetManager_.addWidget(std::unique_ptr<MultiWidget>(
        new MultiWidget(WidgetInterface::Dimensions{0, 209, 430, 60}, 1000)));

    // FPS widget — bottom-right, beside the MultiWidget, tappable to the game
    // screen. Narrow: 50px just fits three NotoSansMono24 digits (14px advance
    // each) plus the border.
    widgetManager_.addWidget(std::unique_ptr<FpsWidget>(new FpsWidget(
        uiController_->getDisplayContext(), WidgetInterface::Dimensions{430, 209, 50, 60}, 250,
        pcMetrics_, EventType::SHOW_GAME,
        [this](EventType action) { this->handleAction(action); })));

    // ── Bottom band: y=269..317, unified across gear / network / clock ──────
    // Settings button — gear icon, no label
    widgetManager_.addWidget(std::unique_ptr<ButtonWidget>(new ButtonWidget(
        uiController_->getDisplayContext(), ButtonIcon::SETTINGS, "",
        WidgetInterface::Dimensions{0, 269, 48, 48}, 0, EventType::SHOW_SETTINGS,
        [this](EventType action) { this->handleAction(action); }, TFT_BLACK, TFT_WHITE)));

    // Network traffic widget — Ethernet up/down rates, right of the settings
    // button. Fills the full 48px band height (like the button) so its two
    // rows (upload/download) have room; ends at x=180 where NetworkWidget
    // begins.
    widgetManager_.addWidget(std::unique_ptr<NetworkTrafficWidget>(
        new NetworkTrafficWidget(WidgetInterface::Dimensions{48, 269, 132, 48}, 1000, pcMetrics_)));

    // Network widget — compact, right-aligned next to clock, vertically
    // centered in the 48px band (269 + (48-24)/2 = 281).
    // Clock: {328, 273, 150, 40} → network widget ends at x=328
    // Width 148 px → x = 328 - 148 = 180
    widgetManager_.addWidget(std::unique_ptr<NetworkWidget>(
        new NetworkWidget(WidgetInterface::Dimensions{180, 281, 148, 24}, 1000, netStatus_)));

    // Clock — taller row (40px) so Mono24 glyphs get vertical padding.
    // Centered on the same band center as NetworkWidget/NetworkTrafficWidget
    // (269 + 48/2 = 293): y = 293 - 40/2 = 273.
    widgetManager_.addWidget(std::unique_ptr<ClockWidget>(new ClockWidget(
        WidgetInterface::Dimensions{328, 273, 150, 40}, 1000, TFT_LIGHTGREY, TFT_BLACK)));
}