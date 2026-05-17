#include "MainScreen.h"

MainScreen::MainScreen(LoggerInterface& logger, PcMetrics& pcMetrics, UiController* uiController,
                       AppConfigInterface& config, ApplicationMetrics& systemMetrics,
                       const AirQualityData& airQualityData,
                       const NetworkStatus& netStatus)
    : BaseWidgetScreen(logger, uiController, config),
      pcMetrics_(pcMetrics),
      systemMetrics_(systemMetrics),
      airQualityData_(airQualityData),
      netStatus_(netStatus) {}

void MainScreen::createWidgets() {
    // PcMetrics
    auto pcMetricsWidget = std::unique_ptr<PcMetricsWidget>(new PcMetricsWidget(
        uiController_->getDisplayContext(), WidgetInterface::Dimensions{0, 0, 480, 150}, 100,
        pcMetrics_, config_, systemMetrics_));
    pcMetricsWidget->setStaleTimeout(5000);
    widgetManager_.addWidget(std::move(pcMetricsWidget));

    // Threads
    auto threadsWidget = std::unique_ptr<ThreadsWidget>(new ThreadsWidget(
        uiController_->getDisplayContext(), WidgetInterface::Dimensions{0, 0, 480 - 86 * 2, 60},
        config_.getHardwareMonitorThreadsRefreshMs(), pcMetrics_, config_, systemMetrics_));
    widgetManager_.addWidget(std::move(threadsWidget));

    // Air quality bar
    widgetManager_.addWidget(std::unique_ptr<AirQualityWidget>(new AirQualityWidget(
        WidgetInterface::Dimensions{0, 150, 480, 44}, 5000, airQualityData_)));

    // Network widget — compact, right-aligned next to clock
    // Clock: {328, 288, 150, 24}  →  network widget ends at x=328
    // Width 148 px → x = 328 - 148 = 180
    widgetManager_.addWidget(std::unique_ptr<NetworkWidget>(new NetworkWidget(
        WidgetInterface::Dimensions{180, 288, 148, 24}, 1000, netStatus_)));

    // Clock
    widgetManager_.addWidget(std::unique_ptr<ClockWidget>(new ClockWidget(
        WidgetInterface::Dimensions{328, 288, 150, 24}, 1000, TFT_LIGHTGREY, TFT_BLACK)));

    // FPS widget
    widgetManager_.addWidget(std::unique_ptr<FpsWidget>(
        new FpsWidget(uiController_->getDisplayContext(),
                      WidgetInterface::Dimensions{400, 200, 72, 72}, 250, pcMetrics_)));

    // Settings button
    widgetManager_.addWidget(std::unique_ptr<ButtonWidget>(new ButtonWidget(
        uiController_->getDisplayContext(), "<", WidgetInterface::Dimensions{0, 272, 48, 48}, 0,
        EventType::SHOW_SETTINGS, [this](EventType action) { this->handleAction(action); },
        TFT_BLACK, TFT_WHITE)));
}