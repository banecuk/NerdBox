#pragma once

#include <cstdint>

#include "services/audio/AudioData.h"
#include "ui/widgets/base/Widget.h"

// Now-playing view for the mb_NerdBox MusicBee plugin push feed (see
// docs-local/NERDBOX_INTEGRATION.md) — title/artist, an elapsed/duration
// progress bar, and a brief "Stopped"/"Disconnected" message on the
// stop/offline events. Hosted by MultiWidget, which owns the switch between
// this and HistorySparklineWidget and the 2s timeout on the stopped message
// — this widget just renders whatever AudioData currently says.
class AudioWidget : public Widget {
 public:
    AudioWidget(const WidgetInterface::Dimensions& dims, uint32_t updateIntervalMs,
                const AudioData& audioData);

 protected:
    void onDraw(bool forceRedraw) override;
    void onDrawStatic() override;

 private:
    static void formatTime(uint32_t ms, char* buf, size_t bufSize);
    static void formatQuality(const AudioData& data, char* buf, size_t bufSize);

    void computeLayout();
    void drawNowPlaying(bool forceRedraw);
    void drawStoppedMessage(bool forceRedraw);

    const AudioData& audioData_;

    // Redraw-skip caches — compared against AudioData each tick so only the
    // parts that actually changed get repainted (see CLAUDE.md's widget
    // memory rules).
    bool firstDraw_ = true;
    bool lastWasStopped_ = false;
    bool lastOffline_ = false;
    char lastTitle_[sizeof(AudioData::title)] = {0};
    char lastArtist_[sizeof(AudioData::artist)] = {0};
    char lastQuality_[24] = {0};
    uint16_t lastBarWidth_ = 0xFFFF;  // sentinel — never a real fill width
    uint32_t lastPositionMs_ = 0xFFFFFFFF;
    uint32_t lastDurationMs_ = 0xFFFFFFFF;
    AudioData::PlayState lastPlayState_ = AudioData::PlayState::Undefined;

    uint16_t titleY_ = 0;
    uint16_t artistY_ = 0;
    uint16_t timeY_ = 0;
    uint16_t barX_ = 0;
    uint16_t barY_ = 0;
    uint16_t barW_ = 0;
    uint16_t barH_ = 0;
};
