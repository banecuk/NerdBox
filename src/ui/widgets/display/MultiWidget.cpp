#include "MultiWidget.h"

#include "ui/widgets/display/AudioWidget.h"
#include "ui/widgets/display/HistorySparklineWidget.h"

MultiWidget::MultiWidget(const WidgetInterface::Dimensions& dims, uint32_t updateIntervalMs,
                         PcMetrics& pcMetrics, const AudioData& audioData,
                         const AppSettings& config)
    : Widget(dims, updateIntervalMs),
      pcMetrics_(pcMetrics),
      audioData_(audioData),
      config_(config) {}

void MultiWidget::onInitialize() {
    candidates_.push_back(
        std::make_unique<HistorySparklineWidget>(dimensions_, updateIntervalMs_, pcMetrics_));
    candidates_.push_back(
        std::make_unique<AudioWidget>(dimensions_, updateIntervalMs_, audioData_));
    for (auto& candidate : candidates_)
        candidate->initialize(getContext());
    activeIndex_ = kSparklineIndex;
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

void MultiWidget::updateActiveCandidate() {
    if (audioData_.hasTrack && audioData_.isPlaying) {
        stoppedShownAtMs_ = 0;
        pausedShownAtMs_ = 0;
        setActiveIndex(kAudioIndex);
        return;
    }

    // Any other non-stopped state with a track loaded — normally Paused,
    // but also covers Loading/Undefined gracefully (e.g. a stray/unexpected
    // playState value) rather than instantly reverting to the sparkline —
    // treated the same as Paused: show it for a bounded window.
    if (audioData_.hasTrack && !audioData_.stopped) {
        stoppedShownAtMs_ = 0;
        const uint32_t now = millis();
        if (pausedShownAtMs_ == 0) {
            // Rising edge into this state — start the timeout window.
            pausedShownAtMs_ = now;
        } else if (now - pausedShownAtMs_ >= config_.audioPausedTimeoutMs) {
            // Window elapsed with no resume — fall back to the sparkline.
            // pausedShownAtMs_ deliberately stays non-zero so this stays
            // stable until playback actually resumes or a new track/stop
            // event resets it.
            setActiveIndex(kSparklineIndex);
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
            // Window elapsed with no new track — fall back to the
            // sparkline. stoppedShownAtMs_ deliberately stays non-zero so
            // this stays stable (no oscillation back to the message) until
            // the next `track` event clears audioData_.stopped.
            setActiveIndex(kSparklineIndex);
            return;
        }
        setActiveIndex(kAudioIndex);
        return;
    }

    stoppedShownAtMs_ = 0;
    pausedShownAtMs_ = 0;
    setActiveIndex(kSparklineIndex);
}

void MultiWidget::onDraw(bool forceRedraw) {
    updateActiveCandidate();
    if (activeIndex_ < candidates_.size())
        candidates_[activeIndex_]->draw(forceRedraw);
    clearDirty();
}

void MultiWidget::onCleanUp() {
    for (auto& candidate : candidates_)
        candidate->cleanUp();
    candidates_.clear();
}

bool MultiWidget::handleTouch(uint16_t x, uint16_t y) {
    if (activeIndex_ < candidates_.size())
        return candidates_[activeIndex_]->handleTouch(x, y);
    return false;
}
