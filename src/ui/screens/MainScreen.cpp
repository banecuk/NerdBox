#include "MainScreen.h"

#include "ui/core/Layout.h"
#include "ui/core/Theme.h"

MainScreen::MainScreen(LoggerInterface& logger, PcMetrics& pcMetrics, UiController* uiController,
                       const AppSettings& config, ApplicationMetrics& systemMetrics,
                       const AirQualityData& airQualityData, const NetworkStatus& netStatus,
                       const AudioData& audioData, WeatherData& weatherData,
                       const RoomClimateData& roomClimateData)
    : BaseWidgetScreen(logger, uiController, config),
      pcMetrics_(pcMetrics),
      systemMetrics_(systemMetrics),
      airQualityData_(airQualityData),
      netStatus_(netStatus),
      audioData_(audioData),
      weatherData_(weatherData),
      roomClimateData_(roomClimateData) {}

void MainScreen::createWidgets() {
    // Threads — reduced-width row filling the left side of the top band, so
    // RoomClimateWidget's 64px column fits between it and AirQualityWidget.
    // 224/28 cores = 8px pitch, unchanged from before the room widget's space
    // was carved out of AirQualityWidget instead.
    auto threadsWidget = std::make_unique<ThreadsWidget>(
        uiController_->getDisplayContext(), WidgetInterface::Dimensions{0, 0, 224, 56},
        config_.hardwareMonitorThreadsRefreshMs, pcMetrics_, config_, systemMetrics_,
        EventType::SHOW_CPU_CLOCK, [this](EventType action) { this->handleAction(action); });
    widgetManager_.addWidget(std::move(threadsWidget), "threads");

    // Room climate — local temperature/humidity sensor, between the threads
    // and air quality blocks on the same top band. One decimal place, always
    // positive (indoor sensor) — worst case "99.9°C" still needs the full
    // 64px column despite never needing a sign.
    widgetManager_.addWidget(
        std::make_unique<RoomClimateWidget>(
            WidgetInterface::Dimensions{224, 0, 64, 56}, 5000, roomClimateData_),
        "room_climate");

    // Air quality block — right of the room climate widget, same top band.
    // Reorganized into four compact columns (icon | temp+humidity |
    // pressure+wind | AQI). Tappable to the weather forecast screen.
    widgetManager_.addWidget(
        std::make_unique<AirQualityWidget>(
            WidgetInterface::Dimensions{288, 0, 192, 56}, 5000, airQualityData_,
            EventType::SHOW_WEATHER, [this](EventType action) { this->handleAction(action); }),
        "air_quality");

    // Game metrics grid — replaces PcMetricsWidget, directly below threads.
    // Moved up (y=56) since the top band got shorter. 84px tall (28px rows,
    // borderMargin=0 on every tile) — see docs-local/10-pcmetrics-tile-height.md.
    // 28px rows (rather than the 26px floor) leave a bit more breathing room
    // around the digits. Tapping any GPU tile opens the game screen
    // (requestScreen() no-ops if already there).
    auto gameMetricsWidget = std::make_unique<PcMetricsWidget>(
        uiController_->getDisplayContext(),
        WidgetInterface::Dimensions{0, 56, Layout::kScreenW, 84}, 100, pcMetrics_,
        EventType::SHOW_GAME, [this](EventType action) { this->handleAction(action); });
    gameMetricsWidget->setStaleTimeout(5000);
    widgetManager_.addWidget(std::move(gameMetricsWidget), "pc_metrics");

    // Multifunctional widget — full screen width now that the FPS tile is
    // gone from the main screen (still shown on the game screen via
    // FpsWidget/GameFpsWidget). Moved up to y=140 (PcMetricsWidget's new
    // bottom edge) and grown to 104px tall, absorbing the remaining 22px
    // freed by PcMetricsWidget's height cut — see
    // docs-local/10-pcmetrics-tile-height.md. Trimmed to 103px to hand the
    // freed pixel to DiskBandWidget's activity lines below.
    widgetManager_.addWidget(
        std::make_unique<MultiWidget>(
            WidgetInterface::Dimensions{0, 140, Layout::kScreenW, 103}, 200, pcMetrics_,
            audioData_, weatherData_, config_, EventType::SHOW_WEATHER,
            [this](EventType action) { this->handleAction(action); }),
        "multi");

    // Disk band — slim strip, tappable to the disk screen. 26px tall: one
    // shared 3px read/write activity line at the top (split left/right, see
    // DiskBandWidget), a 1px gap, then a ~22px borderless per-drive tile area
    // that fits the NotoSans15 value font. Moved below the MultiWidget/FpsWidget
    // row (y=243..269, right above the bottom band).
    widgetManager_.addWidget(
        std::make_unique<DiskBandWidget>(
            uiController_->getDisplayContext(),
            WidgetInterface::Dimensions{0, 243, Layout::kScreenW, 26}, 100, pcMetrics_,
            EventType::SHOW_DISKS, [this](EventType action) { this->handleAction(action); }),
        "disk_band");

    // ── Bottom band is unchanged below this point ──────────────────────────────
    // Same shared band every other screen uses (Layout::kBottomBarY/H) — this
    // used to be a local duplicate 3px higher than the old Layout::kBottomBarY
    // (see N1); now that the shared constant matches this screen's band
    // exactly, there's one source of truth instead of two numbers that had to
    // be kept in sync by hand.
    static constexpr uint16_t kBandY = Layout::kBottomBarY;
    static constexpr uint16_t kBandH = Layout::kBottomBarH;
    static constexpr uint16_t kNetTrafficX = Layout::kButtonSize;
    // Narrower than the band's old undivided 132px — NetworkTrafficWidget's
    // content (two 3-digit-plus-decimal rows and a small arrow) only needs
    // ~65px; the freed space goes to DiskSummaryWidget below.
    static constexpr uint16_t kNetTrafficW = 70;
    static constexpr uint16_t kDiskSumX = kNetTrafficX + kNetTrafficW;
    static constexpr uint16_t kDiskSumW = 88;
    // Narrower than the shared Layout::kClockW (150px, used by every other
    // screen's bottom band) — ClockWidget right-anchors its digits within its
    // own box (see ClockWidget::computeLayout), so shrinking the box only
    // trims the unused space to their left; the digits themselves don't move.
    // The remaining slack, plus what's freed above, covers DiskSumWidget.
    static constexpr uint16_t kClockW = 124;
    static constexpr uint16_t kClockX = Layout::kScreenW - kClockW - 2;
    static constexpr uint16_t kClockH = 40;
    static constexpr uint16_t kClockY = kBandY + kBandH / 2 - kClockH / 2;
    static constexpr uint16_t kNetWidgetH = 24;
    static constexpr uint16_t kNetWidgetY = kBandY + (kBandH - kNetWidgetH) / 2;
    static constexpr uint16_t kNetWidgetW = 148;
    static constexpr uint16_t kNetWidgetX = kClockX - kNetWidgetW;

    // Settings button — gear icon, no label
    widgetManager_.addWidget(
        std::make_unique<ButtonWidget>(
            uiController_->getDisplayContext(), ButtonIcon::SETTINGS, "",
            WidgetInterface::Dimensions{0, kBandY, Layout::kButtonSize, Layout::kButtonSize}, 0,
            EventType::SHOW_SETTINGS, [this](EventType action) { this->handleAction(action); },
            Theme::kSurface, TFT_WHITE),
        "settings_button");

    // Network traffic widget — Ethernet up/down rates, right of the settings
    // button. Fills the full band height (like the button) so its two rows
    // (upload/download) have room; ends where NetworkWidget begins.
    widgetManager_.addWidget(
        std::make_unique<NetworkTrafficWidget>(
            WidgetInterface::Dimensions{kNetTrafficX, kBandY, kNetTrafficW, kBandH}, 1000,
            pcMetrics_),
        "network_traffic");

    // Disk summary widget — sum of read/write rates across all drives, right
    // of the network traffic widget. Same layout/font pattern as
    // NetworkTrafficWidget (two stacked rows), but with "R"/"W" letters
    // instead of direction arrows. Tappable to the disk info screen.
    widgetManager_.addWidget(
        std::make_unique<DiskSummaryWidget>(
            WidgetInterface::Dimensions{kDiskSumX, kBandY, kDiskSumW, kBandH}, 100, pcMetrics_,
            EventType::SHOW_DISKS, [this](EventType action) { this->handleAction(action); }),
        "disk_summary");

    // Network widget — compact, right-aligned next to the clock, vertically
    // centered in the band.
    widgetManager_.addWidget(
        std::make_unique<NetworkWidget>(
            WidgetInterface::Dimensions{kNetWidgetX, kNetWidgetY, kNetWidgetW, kNetWidgetH}, 1000,
            netStatus_),
        "network_status");

    // Clock — taller row so Mono24 glyphs get vertical padding. Centered on
    // the same band center as NetworkWidget/NetworkTrafficWidget. Tappable
    // to the calendar screen.
    widgetManager_.addWidget(
        std::make_unique<ClockWidget>(
            WidgetInterface::Dimensions{kClockX, kClockY, kClockW, kClockH}, 1000, TFT_LIGHTGREY,
            TFT_BLACK, "%H:%M:%S", EventType::SHOW_CALENDAR,
            [this](EventType action) { this->handleAction(action); }),
        "clock");
}