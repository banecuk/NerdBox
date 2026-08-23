#include "MultiWidget.h"

#include "ui/widgets/display/AudioWidget.h"
#include "ui/widgets/display/ForecastStripWidget.h"
#include "ui/widgets/display/HistorySparklineWidget.h"

MultiWidget::MultiWidget(const WidgetInterface::Dimensions& dims, uint32_t updateIntervalMs,
                         PcMetrics& pcMetrics, const AudioData& audioData,
                         WeatherData& weatherData, const AppSettings& config,
                         EventType forecastTapAction,
                         std::function<void(EventType)> forecastTapCallback)
    : Widget(dims, updateIntervalMs),
      pcMetrics_(pcMetrics),
      audioData_(audioData),
      weatherData_(weatherData),
      config_(config),
      forecastTapAction_(forecastTapAction),
      forecastTapCallback_(std::move(forecastTapCallback)) {}

void MultiWidget::onInitialize() {
    auto sparkline =
        std::make_unique<HistorySparklineWidget>(dimensions_, updateIntervalMs_, pcMetrics_);
    sparkline_ = sparkline.get();
    candidates_.push_back(std::move(sparkline));
    candidates_.push_back(
        std::make_unique<AudioWidget>(dimensions_, updateIntervalMs_, audioData_));
    candidates_.push_back(std::make_unique<ForecastStripWidget>(
        dimensions_, updateIntervalMs_, weatherData_, config_, forecastTapAction_,
        forecastTapCallback_));
    for (auto& candidate : candidates_)
        candidate->initialize(getContext());
    activeIndex_ = kForecastIndex;
}

void MultiWidget::onDrawStatic() {
    if (activeIndex_ < candidates_.size())
        candidates_[activeIndex_]->drawStatic();
}

void MultiWidget::setActiveIndex(size_t index) {
    if (index == activeIndex_ || index >= candidates_.size())
        return;
    activeIndex_ = index;
    if (getLcd())
        getLcd()->fillRect(dimensions_.x, dimensions_.y, dimensions_.width, dimensions_.height,
                           TFT_BLACK);
    candidates_[activeIndex_]->drawStatic();
    // Guarantees the newly-active candidate repaints on the very next draw()
    // call regardless of its own needsUpdate() timing.
    candidates_[activeIndex_]->markDirty();
}

size_t MultiWidget::idleCandidate() const {
    if (activity_.isActive())
        return kSparklineIndex;
    if (!(weatherFreshness_.isFresh() && weatherData_.dayCount > 0))
        return kSparklineIndex;
    return kForecastIndex;
}

void MultiWidget::updateActiveCandidate() {
    if (audioData_.hasTrack && audioData_.isPlaying) {
        stoppedShownAtMs_ = 0;
        pausedShownAtMs_ = 0;
        setActiveIndex(kAudioIndex);
        return;
    }

    // Any other non-stopped state with a track loaded — normally Paused,
    // but also covers Loading/Undefined gracefully (e.g. a stray/unexpected
    // playState value) rather than instantly reverting to the idle
    // candidate — treated the same as Paused: show it for a bounded window.
    if (audioData_.hasTrack && !audioData_.stopped) {
        stoppedShownAtMs_ = 0;
        const uint32_t now = millis();
        if (pausedShownAtMs_ == 0) {
            // Rising edge into this state — start the timeout window.
            pausedShownAtMs_ = now;
        } else if (now - pausedShownAtMs_ >= config_.audioPausedTimeoutMs) {
            // Window elapsed with no resume — fall back to the idle
            // candidate. pausedShownAtMs_ deliberately stays non-zero so
            // this stays stable until playback actually resumes or a new
            // track/stop event resets it.
            setActiveIndex(idleCandidate());
            return;
        }
        setActiveIndex(kAudioIndex);
        return;
    }

    if (audioData_.stopped) {
        pausedShownAtMs_ = 0;
        const uint32_t now = millis();
        if (stoppedShownAtMs_ == 0) {
            // Rising edge into the stopped state — start the transient
            // "Stopped"/"Disconnected" message window.
            stoppedShownAtMs_ = now;
        } else if (now - stoppedShownAtMs_ >= config_.audioStoppedMessageMs) {
            // Window elapsed with no new track — fall back to the idle
            // candidate. stoppedShownAtMs_ deliberately stays non-zero so
            // this stays stable (no oscillation back to the message) until
            // the next `track` event clears audioData_.stopped.
            setActiveIndex(idleCandidate());
            return;
        }
        setActiveIndex(kAudioIndex);
        return;
    }

    stoppedShownAtMs_ = 0;
    pausedShownAtMs_ = 0;
    setActiveIndex(idleCandidate());
}

void MultiWidget::onDraw(bool forceRedraw) {
    // Runs every tick regardless of which candidate is visible, so a busy
    // machine is still detected while the forecast strip is showing, and the
    // sparkline's own history keeps advancing while it is hidden (avoids
    // splicing pre-hide samples against post-hide ones with no time gap).
    activity_.tick(millis(), pcMetricsFreshness_.isFresh(), pcMetrics_.cpu_load,
                  pcMetrics_.gpu_load);
    if (sparkline_)
        sparkline_->sampleTick();

    updateActiveCandidate();
    if (activeIndex_ < candidates_.size())
        candidates_[activeIndex_]->draw(forceRedraw);
    clearDirty();
}

void MultiWidget::onCleanUp() {
    for (auto& candidate : candidates_)
        candidate->cleanUp();
    candidates_.clear();
    sparkline_ = nullptr;
}

bool MultiWidget::handleTouch(uint16_t x, uint16_t y) {
    if (activeIndex_ < candidates_.size())
        return candidates_[activeIndex_]->handleTouch(x, y);
    return false;
}
