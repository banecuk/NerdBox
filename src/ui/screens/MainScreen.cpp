#include "MainScreen.h"

#include "ui/core/Layout.h"
#include "ui/core/Theme.h"

MainScreen::MainScreen(LoggerInterface& logger, PcMetrics& pcMetrics, UiController& uiController,
                       const AppSettings& config, ApplicationMetrics& systemMetrics,
                       const AirQualityData& airQualityData, const NetworkStatus& netStatus,
                       const AudioData& audioData, const WeatherData& weatherData,
                       const RoomClimateData& roomClimateData)
    : BaseWidgetScreen(logger, uiController, config),
      pcMetrics_(pcMetrics),
      systemMetrics_(systemMetrics),
      airQualityData_(airQualityData),
      netStatus_(netStatus),
      audioData_(audioData),
      weatherData_(weatherData),
      roomClimateData_(roomClimateData) {}

namespace {
// Band geometry, top to bottom. Each band starts where the previous one
// ends, so changing one height moves everything below it and the
// static_assert catches a stack that no longer meets the bottom band (A18).
constexpr uint16_t kTopBandY = 0;
constexpr uint16_t kTopBandH = 56;
// PcMetricsWidget: 84px tall (28px rows, borderMargin=0 on every tile) —
// see docs-local/10-pcmetrics-tile-height.md.
constexpr uint16_t kPcMetricsY = kTopBandY + kTopBandH;  // 56
constexpr uint16_t kPcMetricsH = 84;
// MultiWidget absorbs whatever PcMetricsWidget/DiskBandWidget don't use.
constexpr uint16_t kMultiY = kPcMetricsY + kPcMetricsH;  // 140
constexpr uint16_t kMultiH = 103;
// DiskBandWidget: one shared 3px read/write activity line, a 1px gap, then a
// ~22px borderless per-drive tile area that fits the NotoSans15 value font.
constexpr uint16_t kDiskBandY = kMultiY + kMultiH;  // 243
constexpr uint16_t kDiskBandH = 26;
static_assert(kDiskBandY + kDiskBandH == Layout::kBottomBarY,
              "MainScreen's bands must tile exactly down to the shared bottom band");

// Top band, left to right: threads | room climate | air quality.
// ThreadsWidget spreads any width % coreCount remainder across its first
// bars' pitch (V20), so kThreadsW needn't divide by Limits::kCores; at the
// default 28 threads it does (8px pitch) and every bar is the same width.
constexpr uint16_t kThreadsW = 224;
constexpr uint16_t kRoomX = kThreadsW;
// One decimal place, always positive (indoor sensor) — worst case "99.9°C"
// still needs the full 64px column despite never needing a sign.
constexpr uint16_t kRoomW = 64;
constexpr uint16_t kAirX = kRoomX + kRoomW;  // 288
constexpr uint16_t kAirW = 192;
static_assert(kThreadsW + kRoomW + kAirW == Layout::kScreenW,
              "MainScreen's top band must span the screen width exactly");
}  // namespace

void MainScreen::createWidgets() {
    // Threads — reduced-width row filling the left side of the top band, so
    // RoomClimateWidget's column fits between it and AirQualityWidget.
    auto threadsWidget = std::make_unique<ThreadsWidget>(
        WidgetInterface::Dimensions{0, kTopBandY, kThreadsW, kTopBandH},
        config_.hardwareMonitorThreadsRefreshMs, pcMetrics_, config_, systemMetrics_,
        tapTo(EventType::SHOW_CPU_CLOCK));
    widgetManager_.addWidget(std::move(threadsWidget), "threads");

    // Room climate — local temperature/humidity sensor, between the threads
    // and air quality blocks on the same top band.
    widgetManager_.addWidget(std::make_unique<RoomClimateWidget>(
                                 WidgetInterface::Dimensions{kRoomX, kTopBandY, kRoomW, kTopBandH},
                                 5000, roomClimateData_, config_.roomClimateStaleTimeoutMs),
                             "room_climate");

    // Air quality block — right of the room climate widget, same top band.
    // Reorganized into four compact columns (icon | temp+humidity |
    // pressure+wind | AQI). Tappable to the weather forecast screen.
    widgetManager_.addWidget(std::make_unique<AirQualityWidget>(
                                 WidgetInterface::Dimensions{kAirX, kTopBandY, kAirW, kTopBandH},
                                 5000, airQualityData_, config_.airQualityStaleTimeoutMs,
                                 tapTo(EventType::SHOW_WEATHER)),
                             "air_quality");

    // Game metrics grid, directly below threads. 28px rows (rather than the
    // 26px floor) leave a bit more breathing room around the digits. Tapping
    // any GPU tile opens the game screen (requestScreen() no-ops if already
    // there). Stale window is PcDataCompositeWidget's default (5 s).
    widgetManager_.addWidget(
        std::make_unique<PcMetricsWidget>(
            WidgetInterface::Dimensions{0, kPcMetricsY, Layout::kScreenW, kPcMetricsH}, 100,
            pcMetrics_, tapTo(EventType::SHOW_GAME)),
        "pc_metrics");

    // Multifunctional widget — full screen width (the FPS tile lives on the
    // game screen only, via FpsWidget/GameFpsWidget), filling the gap
    // between PcMetricsWidget and DiskBandWidget.
    widgetManager_.addWidget(std::make_unique<MultiWidget>(
                                 WidgetInterface::Dimensions{0, kMultiY, Layout::kScreenW, kMultiH},
                                 200, pcMetrics_, audioData_, weatherData_, config_,
                                 tapTo(EventType::SHOW_WEATHER)),
                             "multi");

    // Disk band — slim strip right above the bottom band, tappable to the
    // disk screen (activity line split left/right, see DiskBandWidget).
    widgetManager_.addWidget(
        std::make_unique<DiskBandWidget>(
            WidgetInterface::Dimensions{0, kDiskBandY, Layout::kScreenW, kDiskBandH}, 100,
            pcMetrics_, tapTo(EventType::SHOW_DISKS)),
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
    static constexpr uint16_t kNetWidgetW = 148;
    static constexpr uint16_t kNetWidgetX = kClockX - kNetWidgetW;

    // Settings button — gear icon, no label
    widgetManager_.addWidget(
        std::make_unique<ButtonWidget>(
            ButtonIcon::SETTINGS, "",
            WidgetInterface::Dimensions{0, kBandY, Layout::kButtonSize, Layout::kButtonSize}, 0,
            tapTo(EventType::SHOW_SETTINGS), Theme::kSurface, TFT_WHITE),
        "settings_button");

    // Network traffic widget — Ethernet up/down rates, right of the settings
    // button. Fills the full band height (like the button) so its two rows
    // (upload/download) have room; ends where NetworkWidget begins.
    widgetManager_.addWidget(
        std::make_unique<NetworkTrafficWidget>(
            WidgetInterface::Dimensions{kNetTrafficX, kBandY, kNetTrafficW, kBandH}, 1000,
            pcMetrics_, config_.endpoints.ethUploadMbps, config_.endpoints.ethDownloadMbps),
        "network_traffic");

    // Disk summary widget — sum of read/write rates across all drives, right
    // of the network traffic widget. Same layout/font pattern as
    // NetworkTrafficWidget (two stacked rows), but with "R"/"W" letters
    // instead of direction arrows. Tappable to the disk info screen.
    widgetManager_.addWidget(std::make_unique<DiskSummaryWidget>(
                                 WidgetInterface::Dimensions{kDiskSumX, kBandY, kDiskSumW, kBandH},
                                 100, pcMetrics_, tapTo(EventType::SHOW_DISKS)),
                             "disk_summary");

    // Network widget — compact, right-aligned next to the clock. Hit box
    // spans the full band height (not just the 24px drawn content) so the
    // tap target clears the comfortable-touch floor; NetworkWidget centres
    // its actual drawing within whatever box it's given, so this is a purely
    // additive fix (see docs-local/13-wifi-screen-plan.md §4.5). Tappable to
    // the WIFI screen.
    widgetManager_.addWidget(
        std::make_unique<NetworkWidget>(
            WidgetInterface::Dimensions{kNetWidgetX, kBandY, kNetWidgetW, kBandH}, 1000, netStatus_,
            tapTo(EventType::SHOW_WIFI)),
        "network_status");

    // Clock — taller row so Mono24 glyphs get vertical padding. Centered on
    // the same band center as NetworkWidget/NetworkTrafficWidget. Tappable
    // to the calendar screen.
    widgetManager_.addWidget(std::make_unique<ClockWidget>(
                                 WidgetInterface::Dimensions{kClockX, kClockY, kClockW, kClockH},
                                 1000, TFT_LIGHTGREY, TFT_BLACK, tapTo(EventType::SHOW_CALENDAR)),
                             "clock");
}