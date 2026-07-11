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
    // PcMetrics
    auto pcMetricsWidget = std::unique_ptr<PcMetricsWidget>(new PcMetricsWidget(
        uiController_->getDisplayContext(), WidgetInterface::Dimensions{0, 0, 480, 155}, 100,
        pcMetrics_, config_, systemMetrics_));
    pcMetricsWidget->setStaleTimeout(5000);
    widgetManager_.addWidget(std::move(pcMetricsWidget));

    // Threads
    auto threadsWidget = std::unique_ptr<ThreadsWidget>(new ThreadsWidget(
        uiController_->getDisplayContext(), WidgetInterface::Dimensions{0, 0, 480 - 86 * 2, 60},
        config_.hardwareMonitorThreadsRefreshMs, pcMetrics_, config_, systemMetrics_));
    widgetManager_.addWidget(std::move(threadsWidget));

    // Air quality bar
    widgetManager_.addWidget(std::unique_ptr<AirQualityWidget>(
        new AirQualityWidget(WidgetInterface::Dimensions{0, 155, 480, 44}, 5000, airQualityData_)));

    // Multifunctional widget — below weather, left of FPS, above the bottom band
    widgetManager_.addWidget(std::unique_ptr<MultiWidget>(
        new MultiWidget(WidgetInterface::Dimensions{0, 199, 400, 73}, 1000)));

    // FPS widget — closes the seam with MultiWidget (same top edge, fills to
    // the screen's right edge) and carries a matching border.
    widgetManager_.addWidget(std::unique_ptr<FpsWidget>(
        new FpsWidget(uiController_->getDisplayContext(),
                      WidgetInterface::Dimensions{400, 199, 80, 73}, 250, pcMetrics_)));

    // ── Bottom band: y=272..320, unified across gear / network / clock ──────
    // Settings button — gear icon, no label
    widgetManager_.addWidget(std::unique_ptr<ButtonWidget>(new ButtonWidget(
        uiController_->getDisplayContext(), ButtonIcon::SETTINGS, "",
        WidgetInterface::Dimensions{0, 272, 48, 48}, 0, EventType::SHOW_SETTINGS,
        [this](EventType action) { this->handleAction(action); }, TFT_BLACK, TFT_WHITE)));

    // Network widget — compact, right-aligned next to clock, vertically
    // centered in the 48px band (272 + (48-24)/2 = 284).
    // Clock: {328, 280, 150, 40} → network widget ends at x=328
    // Width 148 px → x = 328 - 148 = 180
    widgetManager_.addWidget(std::unique_ptr<NetworkWidget>(
        new NetworkWidget(WidgetInterface::Dimensions{180, 284, 148, 24}, 1000, netStatus_)));

    // Clock — taller row (40px) so Mono24 glyphs get vertical padding.
    widgetManager_.addWidget(std::unique_ptr<ClockWidget>(new ClockWidget(
        WidgetInterface::Dimensions{328, 280, 150, 40}, 1000, TFT_LIGHTGREY, TFT_BLACK)));
}