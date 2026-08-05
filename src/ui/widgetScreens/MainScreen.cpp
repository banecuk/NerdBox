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
    // Threads — reduced-width row filling the left side of the top band.
    // AirQualityWidget sits to its right at the same height. Both kept short
    // (56px) so downstream rows get more room.
    auto threadsWidget = std::unique_ptr<ThreadsWidget>(new ThreadsWidget(
        uiController_->getDisplayContext(), WidgetInterface::Dimensions{0, 0, 240, 56},
        config_.hardwareMonitorThreadsRefreshMs, pcMetrics_, config_, systemMetrics_));
    widgetManager_.addWidget(std::move(threadsWidget));

    // Air quality block — right of the threads, same top band. Reorganized
    // into four compact columns (icon | temp+humidity | pressure+wind | AQI).
    widgetManager_.addWidget(std::unique_ptr<AirQualityWidget>(
        new AirQualityWidget(WidgetInterface::Dimensions{240, 0, 240, 56}, 5000,
                             airQualityData_)));

    // Game metrics grid — replaces PcMetricsWidget, directly below threads.
    // Moved up (y=56) since the top band got shorter.
    auto gameMetricsWidget = std::unique_ptr<PcMetricsWidget>(
        new PcMetricsWidget(uiController_->getDisplayContext(),
                              WidgetInterface::Dimensions{0, 56, 480, 106}, 100, pcMetrics_));
    gameMetricsWidget->setStaleTimeout(5000);
    widgetManager_.addWidget(std::move(gameMetricsWidget));

    // Disk band — slim strip, tappable to the disk screen. 27px tall: 4px
    // read/write activity lines + a ~19px borderless per-drive tile area that
    // fits the NotoSans15 value font and runs flush against both lines.
    widgetManager_.addWidget(std::unique_ptr<DiskBandWidget>(new DiskBandWidget(
        uiController_->getDisplayContext(), WidgetInterface::Dimensions{0, 162, 480, 27}, 100,
        pcMetrics_, EventType::SHOW_DISKS,
        [this](EventType action) { this->handleAction(action); })));

    // Multifunctional widget — left of the FPS display; width trimmed from the
    // right to make room for a minimal-width FPS tile beside it.
    // Taller now (y=189..269) to fill the space freed by the shorter top rows.
    widgetManager_.addWidget(std::unique_ptr<MultiWidget>(
        new MultiWidget(WidgetInterface::Dimensions{0, 189, 430, 80}, 1000)));

    // FPS widget — bottom-right, beside the MultiWidget, tappable to the
    // game screen. Narrow: 50px just fits three NotoSansMono24 digits (14px
    // advance each) plus the border. Grown to match the MultiWidget height.
    widgetManager_.addWidget(std::unique_ptr<FpsWidget>(new FpsWidget(
        uiController_->getDisplayContext(), WidgetInterface::Dimensions{430, 189, 50, 80}, 250,
        pcMetrics_, EventType::SHOW_GAME,
        [this](EventType action) { this->handleAction(action); })));

    // ── Bottom band is unchanged below this point ──────────────────────────────
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