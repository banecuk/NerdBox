#include "MainScreen.h"

MainScreen::MainScreen(LoggerInterface& logger, PcMetrics& pcMetrics, UiController* uiController,
                       AppConfigInterface& config, ApplicationMetrics& systemMetrics)
    : BaseWidgetScreen(logger, uiController, config),
      pcMetrics_(pcMetrics),
      systemMetrics_(systemMetrics) {}

void MainScreen::createWidgets() {
    // Add PcMetricsWidget (without ThreadsWidget inside)
    auto pcMetricsWidget = std::unique_ptr<PcMetricsWidget>(new PcMetricsWidget(
        uiController_->getDisplayContext(), WidgetInterface::Dimensions{0, 0, 480, 120}, 100,
        pcMetrics_, config_, systemMetrics_));
    pcMetricsWidget->setStaleTimeout(5000);
    widgetManager_.addWidget(std::move(pcMetricsWidget));

    // Add ThreadsWidget as a separate top-level widget for independent 60 FPS updates
    auto threadsWidget = std::unique_ptr<ThreadsWidget>(new ThreadsWidget(
        uiController_->getDisplayContext(), WidgetInterface::Dimensions{0, 0, 480 - 86 * 2, 60},
        config_.getHardwareMonitorThreadsRefreshMs(), pcMetrics_, config_, systemMetrics_));
    widgetManager_.addWidget(std::move(threadsWidget));

    // Clock widget
    widgetManager_.addWidget(std::unique_ptr<ClockWidget>(new ClockWidget(
        WidgetInterface::Dimensions{328, 288, 150, 24}, 1000, TFT_LIGHTGREY, TFT_BLACK, 3)));

    // Settings button
    widgetManager_.addWidget(std::unique_ptr<ButtonWidget>(new ButtonWidget(
        uiController_->getDisplayContext(), "<", WidgetInterface::Dimensions{0, 272, 48, 48}, 0,
        EventType::SHOW_SETTINGS, [this](EventType action) { this->handleAction(action); },
        TFT_BLACK, TFT_WHITE)));
}