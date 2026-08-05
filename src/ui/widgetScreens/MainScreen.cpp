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
    // Threads — top-left, unchanged
    auto threadsWidget = std::unique_ptr<ThreadsWidget>(new ThreadsWidget(
        uiController_->getDisplayContext(), WidgetInterface::Dimensions{0, 0, 308, 60},
        config_.hardwareMonitorThreadsRefreshMs, pcMetrics_, config_, systemMetrics_));
    widgetManager_.addWidget(std::move(threadsWidget));

    // FPS widget — top-right corner, tappable to the game screen
    widgetManager_.addWidget(std::unique_ptr<FpsWidget>(new FpsWidget(
        uiController_->getDisplayContext(), WidgetInterface::Dimensions{308, 0, 172, 60}, 250,
        pcMetrics_, EventType::SHOW_GAME,
        [this](EventType action) { this->handleAction(action); })));

    // Game metrics grid — replaces PcMetricsWidget, directly below threads.
    // 78px tall → 26px tile rows (1px more top and bottom per tile vs
    // GameScreen's 90px → 30px) via GameMetricsWidget::rowHeight() rescaling.
    auto gameMetricsWidget = std::unique_ptr<GameMetricsWidget>(
        new GameMetricsWidget(uiController_->getDisplayContext(),
                              WidgetInterface::Dimensions{0, 60, 480, 78}, 100, pcMetrics_));
    gameMetricsWidget->setStaleTimeout(5000);
    widgetManager_.addWidget(std::move(gameMetricsWidget));

    // Disk band — slim strip, tappable to the disk screen. 30px tall: 4px
    // read/write activity lines (doubled from 2px) + a ~21px per-drive tile
    // area that fits the NotoSans18 value font.
    widgetManager_.addWidget(std::unique_ptr<DiskBandWidget>(new DiskBandWidget(
        uiController_->getDisplayContext(), WidgetInterface::Dimensions{0, 138, 480, 30}, 100,
        pcMetrics_, EventType::SHOW_DISKS,
        [this](EventType action) { this->handleAction(action); })));

    // Air quality bar
    widgetManager_.addWidget(std::unique_ptr<AirQualityWidget>(
        new AirQualityWidget(WidgetInterface::Dimensions{0, 168, 480, 44}, 5000, airQualityData_)));

    // Multifunctional widget — full width, fills the bottom of the content area
    widgetManager_.addWidget(std::unique_ptr<MultiWidget>(
        new MultiWidget(WidgetInterface::Dimensions{0, 212, 480, 60}, 1000)));

    // ── Bottom band: y=272..320, unified across gear / network / clock ──────
    // Settings button — gear icon, no label
    widgetManager_.addWidget(std::unique_ptr<ButtonWidget>(new ButtonWidget(
        uiController_->getDisplayContext(), ButtonIcon::SETTINGS, "",
        WidgetInterface::Dimensions{0, 272, 48, 48}, 0, EventType::SHOW_SETTINGS,
        [this](EventType action) { this->handleAction(action); }, TFT_BLACK, TFT_WHITE)));

    // Network traffic widget — Ethernet up/down rates, right of the settings
    // button. Fills the full 48px band height (like the button) so its two
    // rows (upload/download) have room; ends at x=180 where NetworkWidget
    // begins.
    widgetManager_.addWidget(std::unique_ptr<NetworkTrafficWidget>(
        new NetworkTrafficWidget(WidgetInterface::Dimensions{48, 272, 132, 48}, 1000, pcMetrics_)));

    // Network widget — compact, right-aligned next to clock, vertically
    // centered in the 48px band (272 + (48-24)/2 = 284).
    // Clock: {328, 280, 150, 40} → network widget ends at x=328
    // Width 148 px → x = 328 - 148 = 180
    widgetManager_.addWidget(std::unique_ptr<NetworkWidget>(
        new NetworkWidget(WidgetInterface::Dimensions{180, 284, 148, 24}, 1000, netStatus_)));

    // Clock — taller row (40px) so Mono24 glyphs get vertical padding.
    // Centered on the same band center as NetworkWidget/NetworkTrafficWidget
    // (272 + 48/2 = 296): y = 296 - 40/2 = 276.
    widgetManager_.addWidget(std::unique_ptr<ClockWidget>(new ClockWidget(
        WidgetInterface::Dimensions{328, 276, 150, 40}, 1000, TFT_LIGHTGREY, TFT_BLACK)));
}