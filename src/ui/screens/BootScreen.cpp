#include "BootScreen.h"

#include <cstdio>
#include <cstring>

#include "ui/core/Colors.h"
#include "ui/resources/FontRegistry.h"

namespace {
uint16_t colorForLevel(LogLevel level) {
    switch (level) {
        case LogLevel::WARNING:
            return Colors::kWarn;
        case LogLevel::ERROR:
        case LogLevel::CRITICAL:
            return Colors::kDanger;
        case LogLevel::INFO:
        case LogLevel::DEBUG:
        default:
            return Colors::kDimLabelGrey;
    }
}
}  // namespace

BootScreen::BootScreen(ScreenLogQueue& screenLogQueue, LGFX* lcd)
    : screenLogQueue_(screenLogQueue), lcd_(lcd) {}

void BootScreen::onEnter() {
    if (!lcd_)
        return;

    lcd_->fillScreen(TFT_BLACK);

    drawWordmark();

    // Measure label font height once so log lines are spaced correctly.
    Fonts::loadLabel(lcd_);
    lineHeight_ = static_cast<uint16_t>(lcd_->fontHeight()) + 2;
    Fonts::unload(lcd_);

    lineY_ = kLogAreaY;

    hasStatusMessage_ = false;
    hasPinnedMessage_ = false;
}

void BootScreen::onExit() {}

void BootScreen::draw() {
    if (!lcd_)
        return;

    char message[200];
    LogLevel level;
    // Only repaint the header lines when a new message changed them. They
    // used to be cleared and redrawn every 16ms tick, which flickered
    // visibly while boot sat on one step (e.g. "Syncing time" retries).
    bool statusChanged = false;
    bool pinnedChanged = false;
    while (screenLogQueue_.popScreenMessage(message, sizeof(message), &level)) {
        // Rolling status line — always the most recently popped message.
        strncpy(statusMessage_, message, sizeof(statusMessage_) - 1);
        statusMessage_[sizeof(statusMessage_) - 1] = '\0';
        statusLevel_ = level;
        hasStatusMessage_ = true;
        statusChanged = true;

        // Pin the most recent WARN+ line so it can't scroll away beneath
        // later INFO chatter (e.g. a WiFi failure followed by NTP retries).
        if (level == LogLevel::WARNING || level == LogLevel::ERROR ||
            level == LogLevel::CRITICAL) {
            strncpy(pinnedMessage_, message, sizeof(pinnedMessage_) - 1);
            pinnedMessage_[sizeof(pinnedMessage_) - 1] = '\0';
            pinnedLevel_ = level;
            hasPinnedMessage_ = true;
            pinnedChanged = true;
        }

        // Once the log area is full, wipe it and start again from the top
        // instead of drawing past the bottom of the screen. (scroll() can't
        // be used: the panel is configured non-readable, so pixels can't be
        // copied back, and it would move the header too.)
        if (lineY_ + lineHeight_ > lcd_->height()) {
            lcd_->fillRect(0, kLogAreaY, lcd_->width(), lcd_->height() - kLogAreaY, TFT_BLACK);
            lineY_ = kLogAreaY;
        }

        // Demoted: small, dim — the important bit now lives in the status/
        // pinned lines above, not in scanning the scrollback.
        Fonts::loadLabel(lcd_);
        lcd_->setTextColor(TFT_DARKGREY, TFT_BLACK);
        lcd_->setTextDatum(TL_DATUM);
        lcd_->drawString(message, 0, lineY_);
        Fonts::unload(lcd_);

        lineY_ += lineHeight_;
    }

    if (statusChanged)
        drawStatusLine();
    if (pinnedChanged)
        drawPinnedLine();
}

// ---------------------------------------------------------------------------
// Header renderers
// ---------------------------------------------------------------------------

void BootScreen::drawWordmark() {
    // Centred wordmark, shadow offset by 1px for depth (was top-left before
    // — see docs-local/03-visual-ux.md V10).
    const int16_t cx = lcd_->width() / 2;

    Fonts::loadMetric(lcd_);
    lcd_->setTextDatum(TC_DATUM);
    lcd_->setTextColor(TFT_DARKGRAY, TFT_BLACK);
    lcd_->drawString("NerdBox", cx + 1, kWordmarkY + 1);
    lcd_->setTextColor(TFT_DARKCYAN, TFT_BLACK);
    lcd_->drawString("NerdBox", cx, kWordmarkY);
    Fonts::unload(lcd_);

    // Build string — compile date doubles as a version stamp; DEBUG_MODE is
    // already the build's own identity (see CLAUDE.md's build system table).
#if DEBUG_MODE
    static constexpr const char* kBuildKind = "DEBUG";
#else
    static constexpr const char* kBuildKind = "RELEASE";
#endif
    char buildLine[40];
    snprintf(buildLine, sizeof(buildLine), "%s build \xc2\xb7 %s", kBuildKind, __DATE__);

    Fonts::loadLabel(lcd_);
    lcd_->setTextDatum(TC_DATUM);
    lcd_->setTextColor(Colors::kDimLabelGrey, TFT_BLACK);
    lcd_->drawString(buildLine, cx, kBuildLineY);
    Fonts::unload(lcd_);
}

void BootScreen::drawStatusLine() {
    lcd_->fillRect(0, kStatusLineY, lcd_->width(), kPinnedLineY - kStatusLineY, TFT_BLACK);

    Fonts::loadLabel(lcd_);
    lcd_->setTextDatum(TL_DATUM);
    lcd_->setTextColor(colorForLevel(statusLevel_), TFT_BLACK);
    lcd_->drawString(statusMessage_, 8, kStatusLineY);
    Fonts::unload(lcd_);
}

void BootScreen::drawPinnedLine() {
    lcd_->fillRect(0, kPinnedLineY, lcd_->width(), kLogAreaY - kPinnedLineY, TFT_BLACK);

    char line[204];
    snprintf(line, sizeof(line), "! %s", pinnedMessage_);

    Fonts::loadLabel(lcd_);
    lcd_->setTextDatum(TL_DATUM);
    lcd_->setTextColor(colorForLevel(pinnedLevel_), TFT_BLACK);
    lcd_->drawString(line, 8, kPinnedLineY);
    Fonts::unload(lcd_);
}
