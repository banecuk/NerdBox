#pragma once

#include <functional>
#include <memory>
#include <vector>

#include "config/AppSettings.h"
#include "core/events/EventTypes.h"
#include "services/audio/AudioData.h"
#include "services/pcMetrics/PcMetrics.h"
#include "services/weather/WeatherData.h"
#include "ui/widgets/base/Widget.h"
#include "utils/ActivityDetector.h"
#include "utils/DataFreshnessGuard.h"

class HistorySparklineWidget;

// Multifunctional content area on MainScreen — sits below the AirQualityWidget,
// left of the FpsWidget, and above the ClockWidget / NetworkWidget row.
// Container: owns candidate sub-widgets and shows whichever one is active, in
// priority order:
//   1. AudioWidget — any audio info available (playing, paused-window, or
//      stop/offline message window). Unchanged rules:
//      - Playing -> indefinitely.
//      - Paused -> for config_.audioPausedTimeoutMs, then falls back to the
//        idle candidate (2 or 3 below) unless playback resumes first.
//      - stop/offline -> a transient "Stopped"/"Disconnected" message for
//        config_.audioStoppedMessageMs, then falls back to the idle
//        candidate unless a new track starts first.
//   2. HistorySparklineWidget — no audio, and PC metrics show high activity
//      (ActivityDetector, hysteresis over cpu_load/gpu_load).
//   3. ForecastStripWidget (default/resting state) — no audio, machine idle.
//      Falls back to 2 if the forecast has no data (never fetched, or stale).
class MultiWidget : public Widget {
 public:
    MultiWidget(const WidgetInterface::Dimensions& dims, uint32_t updateIntervalMs,
                PcMetrics& pcMetrics, const AudioData& audioData, WeatherData& weatherData,
                const AppSettings& config, EventType forecastTapAction = EventType::NONE,
                std::function<void(EventType)> forecastTapCallback = nullptr);

    bool handleTouch(uint16_t x, uint16_t y) override;

 protected:
    void onInitialize() override;
    void onDraw(bool forceRedraw) override;
    void onDrawStatic() override;
    void onCleanUp() override;

 private:
    static constexpr size_t kSparklineIndex = 0;
    static constexpr size_t kAudioIndex = 1;
    static constexpr size_t kForecastIndex = 2;

    void updateActiveCandidate();
    void setActiveIndex(size_t index);
    // The non-audio resting choice: sparkline while the machine is busy,
    // otherwise the forecast strip — falling back to the sparkline if the
    // forecast has nothing to show (never fetched, or stale).
    size_t idleCandidate() const;

    PcMetrics& pcMetrics_;
    const AudioData& audioData_;
    WeatherData& weatherData_;
    const AppSettings& config_;
    EventType forecastTapAction_;
    std::function<void(EventType)> forecastTapCallback_;

    std::vector<std::unique_ptr<WidgetInterface>> candidates_;
    // Raw pointer into candidates_[kSparklineIndex] — sampleTick() must run
    // every tick regardless of which candidate is visible, so it can't be
    // reached only through the WidgetInterface pointer.
    HistorySparklineWidget* sparkline_ = nullptr;
    size_t activeIndex_ = 0;

    // Own freshness guard over WeatherData rather than reaching into
    // ForecastStripWidget to ask it — keeps the priority decision here.
    DataFreshnessGuard weatherFreshness_{weatherData_.freshness, config_.weatherRefreshIntervalMs};
    // Default timeout — same contract HistorySparklineWidget itself uses to
    // decide when PcMetrics is too stale to plot.
    DataFreshnessGuard pcMetricsFreshness_{pcMetrics_.freshness};

    ActivityDetector activity_{
        config_.multiWidgetActivityEnterPct, config_.multiWidgetActivityExitPct,
        config_.multiWidgetActivityQuietMs, config_.multiWidgetActivityEnterHoldMs};

    // 0 = not currently in the post-stop transient window; otherwise the
    // millis() timestamp the stop/offline message started being shown.
    uint32_t stoppedShownAtMs_ = 0;

    // 0 = not currently timing a paused track; otherwise the millis()
    // timestamp playback was last seen entering/staying in Paused.
    uint32_t pausedShownAtMs_ = 0;
};
