#pragma once

#include <memory>
#include <vector>

#include "config/AppSettings.h"
#include "services/audio/AudioData.h"
#include "services/pcMetrics/PcMetrics.h"
#include "ui/widgets/base/Widget.h"

// Multifunctional content area on MainScreen — sits below the AirQualityWidget,
// left of the FpsWidget, and above the ClockWidget / NetworkWidget row.
// Container: owns candidate sub-widgets and shows whichever one is active,
// switching based on AudioData:
//   - Playing -> AudioWidget's now-playing view, indefinitely.
//   - Paused -> AudioWidget's now-playing view for config_.audioPausedTimeoutMs,
//     then falls back to HistorySparklineWidget unless playback resumes first.
//   - stop/offline -> AudioWidget's transient "Stopped"/"Disconnected"
//     message for config_.audioStoppedMessageMs, then falls back to
//     HistorySparklineWidget unless a new track starts first.
//   - No track ever seen -> HistorySparklineWidget (the original, and still
//     the default, candidate).
class MultiWidget : public Widget {
 public:
    MultiWidget(const WidgetInterface::Dimensions& dims, uint32_t updateIntervalMs,
                PcMetrics& pcMetrics, const AudioData& audioData, const AppSettings& config);

    bool handleTouch(uint16_t x, uint16_t y) override;

 protected:
    void onInitialize() override;
    void onDraw(bool forceRedraw) override;
    void onDrawStatic() override;
    void onCleanUp() override;

 private:
    static constexpr size_t kSparklineIndex = 0;
    static constexpr size_t kAudioIndex = 1;

    void updateActiveCandidate();
    void setActiveIndex(size_t index);

    PcMetrics& pcMetrics_;
    const AudioData& audioData_;
    const AppSettings& config_;
    std::vector<std::unique_ptr<WidgetInterface>> candidates_;
    size_t activeIndex_ = 0;

    // 0 = not currently in the post-stop transient window; otherwise the
    // millis() timestamp the stop/offline message started being shown.
    uint32_t stoppedShownAtMs_ = 0;

    // 0 = not currently timing a paused track; otherwise the millis()
    // timestamp playback was last seen entering/staying in Paused.
    uint32_t pausedShownAtMs_ = 0;
};
