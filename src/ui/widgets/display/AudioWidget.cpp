#include "AudioWidget.h"

#include <cctype>
#include <cstdio>
#include <cstring>

#include "ui/core/UiText.h"
#include "ui/resources/FontRegistry.h"

namespace {
constexpr uint16_t kTitleRowH = 20;
constexpr uint16_t kArtistRowH = 16;
constexpr uint16_t kAlbumRowH = 14;
constexpr uint16_t kTimeRowH = 14;
// Gap between the quality label and the time text on the same row.
constexpr int32_t kQualityTimeGap = 10;
constexpr uint16_t kBarH = 8;
// Deliberately no left padding — the widget sits flush against the left
// edge of its slot. kRightPadding keeps right-aligned text/the progress bar
// off the right edge only.
constexpr uint16_t kRightPadding = 8;

constexpr uint16_t kBarBgColor = 0x2965;      // dark grey-blue track
constexpr uint16_t kBarColorPlaying = 0x4FFF;  // bright cyan fill
constexpr uint16_t kBarColorPaused = 0x2D5B;   // dimmed version of the above
}  // namespace

AudioWidget::AudioWidget(const WidgetInterface::Dimensions& dims, uint32_t updateIntervalMs,
                         const AudioData& audioData)
    : Widget(dims, updateIntervalMs), audioData_(audioData) {}

void AudioWidget::computeLayout() {
    titleY_ = dimensions_.y + 2;
    artistY_ = titleY_ + kTitleRowH + 1;
    albumY_ = artistY_ + kArtistRowH + 1;
    timeY_ = albumY_ + kAlbumRowH + 2;
    barH_ = kBarH;
    barY_ = timeY_ + kTimeRowH + 2;
    barX_ = dimensions_.x;
    barW_ = dimensions_.width > kRightPadding ? dimensions_.width - kRightPadding : 0;
}

void AudioWidget::formatTime(uint32_t ms, char* buf, size_t bufSize) {
    const uint32_t totalSeconds = ms / 1000;
    const uint32_t minutes = totalSeconds / 60;
    const uint32_t seconds = totalSeconds % 60;
    snprintf(buf, bufSize, "%lu:%02lu", static_cast<unsigned long>(minutes),
             static_cast<unsigned long>(seconds));
}

void AudioWidget::formatQuality(const AudioData& data, char* buf, size_t bufSize) {
    char formatUpper[sizeof(AudioData::format)];
    size_t i = 0;
    for (; data.format[i] != '\0' && i + 1 < sizeof(formatUpper); ++i)
        formatUpper[i] = static_cast<char>(toupper(static_cast<unsigned char>(data.format[i])));
    formatUpper[i] = '\0';

    if (formatUpper[0] == '\0') {
        buf[0] = '\0';
        return;
    }

    // Lossless formats are conventionally shown by name alone ("FLAC");
    // lossy ones pair the format with its bitrate ("MP3 256").
    const bool lossless = strcmp(formatUpper, "FLAC") == 0 || strcmp(formatUpper, "WAV") == 0 ||
                          strcmp(formatUpper, "APE") == 0 || strcmp(formatUpper, "ALAC") == 0;

    if (!lossless && data.bitrate[0] != '\0') {
        snprintf(buf, bufSize, "%s %s", formatUpper, data.bitrate);
    } else {
        snprintf(buf, bufSize, "%s", formatUpper);
    }
}

void AudioWidget::onDrawStatic() {
    getLcd()->fillRect(dimensions_.x, dimensions_.y, dimensions_.width, dimensions_.height,
                       TFT_BLACK);
    computeLayout();
    firstDraw_ = true;
}

void AudioWidget::onDraw(bool forceRedraw) {
    if (!getLcd())
        return;

    const bool stoppedView = audioData_.stopped;
    if (forceRedraw || firstDraw_ || stoppedView != lastWasStopped_) {
        getLcd()->fillRect(dimensions_.x, dimensions_.y, dimensions_.width, dimensions_.height,
                           TFT_BLACK);
        forceRedraw = true;
    }
    lastWasStopped_ = stoppedView;
    firstDraw_ = false;

    if (stoppedView) {
        drawStoppedMessage(forceRedraw);
    } else {
        drawNowPlaying(forceRedraw);
    }
    clearDirty();
}

void AudioWidget::drawStoppedMessage(bool forceRedraw) {
    const bool offline = audioData_.offline;
    if (!forceRedraw && offline == lastOffline_)
        return;
    lastOffline_ = offline;

    char posBuf[16];
    formatTime(audioData_.positionMs, posBuf, sizeof(posBuf));
    char line2[128];
    snprintf(line2, sizeof(line2), "%s at %s", audioData_.title[0] ? audioData_.title : "-",
             posBuf);

    LGFX* lcd = getLcd();
    const int32_t cx = dimensions_.x + dimensions_.width / 2;
    const int32_t cy = dimensions_.y + dimensions_.height / 2;

    lcd->setClipRect(dimensions_.x, dimensions_.y, dimensions_.width, dimensions_.height);

    Fonts::loadValue(lcd);
    lcd->setTextColor(offline ? TFT_RED : TFT_ORANGE, TFT_BLACK);
    lcd->setTextDatum(MC_DATUM);
    lcd->drawString(offline ? "Disconnected" : "Stopped", cx, cy - 10);
    Fonts::unload(lcd);

    Fonts::loadLabel(lcd);
    lcd->setTextColor(TFT_DARKGREY, TFT_BLACK);
    lcd->drawString(line2, cx, cy + 14);
    Fonts::unload(lcd);

    lcd->clearClipRect();
}

void AudioWidget::drawNowPlaying(bool forceRedraw) {
    LGFX* lcd = getLcd();

    const bool titleChanged = forceRedraw || strcmp(audioData_.title, lastTitle_) != 0;
    const bool artistChanged = forceRedraw || strcmp(audioData_.artist, lastArtist_) != 0;
    const bool albumChanged = forceRedraw || strcmp(audioData_.album, lastAlbum_) != 0;

    if (titleChanged) {
        lcd->fillRect(dimensions_.x, titleY_, dimensions_.width, kTitleRowH, TFT_BLACK);
        lcd->setClipRect(dimensions_.x, titleY_, dimensions_.width, kTitleRowH);
        Fonts::loadValue(lcd);
        lcd->setTextColor(TFT_WHITE, TFT_BLACK);
        lcd->setTextDatum(TL_DATUM);
        lcd->drawString(audioData_.title[0] ? audioData_.title : UiText::kNoData, dimensions_.x,
                        titleY_);
        Fonts::unload(lcd);
        lcd->clearClipRect();
        snprintf(lastTitle_, sizeof(lastTitle_), "%s", audioData_.title);
    }

    if (artistChanged) {
        lcd->fillRect(dimensions_.x, artistY_, dimensions_.width, kArtistRowH, TFT_BLACK);
        lcd->setClipRect(dimensions_.x, artistY_, dimensions_.width, kArtistRowH);
        Fonts::loadLabel(lcd);
        lcd->setTextColor(TFT_LIGHTGREY, TFT_BLACK);
        lcd->setTextDatum(TL_DATUM);
        lcd->drawString(audioData_.artist, dimensions_.x, artistY_);
        Fonts::unload(lcd);
        lcd->clearClipRect();
        snprintf(lastArtist_, sizeof(lastArtist_), "%s", audioData_.artist);
    }

    if (albumChanged) {
        lcd->fillRect(dimensions_.x, albumY_, dimensions_.width, kAlbumRowH, TFT_BLACK);
        lcd->setClipRect(dimensions_.x, albumY_, dimensions_.width, kAlbumRowH);
        Fonts::loadLabel(lcd);
        lcd->setTextColor(TFT_DARKGREY, TFT_BLACK);
        lcd->setTextDatum(TL_DATUM);
        lcd->drawString(audioData_.album, dimensions_.x, albumY_);
        Fonts::unload(lcd);
        lcd->clearClipRect();
        snprintf(lastAlbum_, sizeof(lastAlbum_), "%s", audioData_.album);
    }

    const uint32_t duration = audioData_.durationMs;
    // Only clamp against duration when it's known — a duration of 0 means
    // "unknown length" (e.g. a live stream), not "already at the end", so
    // positionMs should still advance the displayed elapsed time even though
    // the bar itself has nothing to fill against.
    const uint32_t position =
        (duration > 0 && audioData_.positionMs > duration) ? duration : audioData_.positionMs;
    const uint16_t fillWidth =
        (duration > 0 && barW_ > 0)
            ? static_cast<uint16_t>((static_cast<uint64_t>(position) * barW_) / duration)
            : 0;
    const bool playing = audioData_.isPlaying;
    const uint16_t barColor = playing ? kBarColorPlaying : kBarColorPaused;

    // Progress bar: only repaint the delta segment between the last drawn
    // width and the new one, instead of clearing/repainting the whole bar
    // every ~1s tick — the full-bar fillRect was visibly flickering. A full
    // repaint only happens when play/pause changes the fill color (or on a
    // forced redraw).
    if (forceRedraw || audioData_.playState != lastPlayState_) {
        lcd->fillRect(barX_, barY_, barW_, barH_, kBarBgColor);
        if (fillWidth > 0)
            lcd->fillRect(barX_, barY_, fillWidth, barH_, barColor);
    } else if (fillWidth != lastBarWidth_) {
        if (fillWidth > lastBarWidth_)
            lcd->fillRect(barX_ + lastBarWidth_, barY_, fillWidth - lastBarWidth_, barH_, barColor);
        else
            lcd->fillRect(barX_ + fillWidth, barY_, lastBarWidth_ - fillWidth, barH_, kBarBgColor);
    }
    lastBarWidth_ = fillWidth;
    lastPlayState_ = audioData_.playState;

    // Time (right-aligned, white) with the quality label immediately to its
    // left (grey) — both share one row instead of a separate quality line.
    char quality[24];
    formatQuality(audioData_, quality, sizeof(quality));
    const bool qualityChanged = strcmp(quality, lastQuality_) != 0;

    if (forceRedraw || position != lastPositionMs_ || duration != lastDurationMs_ || qualityChanged) {
        char elapsed[16];
        char total[16];
        formatTime(position, elapsed, sizeof(elapsed));
        formatTime(duration, total, sizeof(total));
        char timeLine[40];
        snprintf(timeLine, sizeof(timeLine), "%s / %s", elapsed, total);

        lcd->fillRect(dimensions_.x, timeY_, dimensions_.width, kTimeRowH, TFT_BLACK);
        lcd->setClipRect(dimensions_.x, timeY_, dimensions_.width, kTimeRowH);
        Fonts::loadLabel(lcd);

        const int32_t timeRightX = dimensions_.x + dimensions_.width - kRightPadding;
        lcd->setTextColor(TFT_WHITE, TFT_BLACK);
        lcd->setTextDatum(TR_DATUM);
        lcd->drawString(timeLine, timeRightX, timeY_);

        if (quality[0]) {
            const int32_t timeWidth = lcd->textWidth(timeLine);
            lcd->setTextColor(TFT_DARKGREY, TFT_BLACK);
            lcd->drawString(quality, timeRightX - timeWidth - kQualityTimeGap, timeY_);
        }

        Fonts::unload(lcd);
        lcd->clearClipRect();

        lastPositionMs_ = position;
        lastDurationMs_ = duration;
        snprintf(lastQuality_, sizeof(lastQuality_), "%s", quality);
    }
}
