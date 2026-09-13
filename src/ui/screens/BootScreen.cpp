#include "BootScreen.h"

#include <cstdio>
#include <cstring>

#include "ui/core/Colors.h"
#include "ui/core/Theme.h"
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

BootScreen::BootScreen(ScreenLogQueue& screenLogQueue, const SystemState::CoreState& coreState,
                       LGFX* lcd)
    : screenLogQueue_(screenLogQueue), coreState_(coreState), lcd_(lcd) {}

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
    lastDrawnPercent_ = 0xFF;
    drawProgressBar();

    // Confine scroll() (called from draw() once the log area fills up) to
    // the log area only, so it never disturbs the header above it.
    lcd_->setClipRect(0, kLogAreaY, lcd_->width(), lcd_->height() - kLogAreaY);
}

void BootScreen::onExit() {
    if (lcd_)
        lcd_->clearClipRect();
}

void BootScreen::draw() {
    if (!lcd_)
        return;

    // Progress bar is driven by shared state, not the log queue — a phase
    // transition doesn't always coincide with a forScreen log message, so
    // this is checked every tick rather than only when one pops below.
    if (coreState_.bootProgressPercent != lastDrawnPercent_) {
        drawProgressBar();
    }

    char message[200];
    LogLevel level;
    while (screenLogQueue_.popScreenMessage(message, sizeof(message), &level)) {
        // Rolling status line — always the most recently popped message.
        strncpy(statusMessage_, message, sizeof(statusMessage_) - 1);
        statusMessage_[sizeof(statusMessage_) - 1] = '\0';
        statusLevel_ = level;
        hasStatusMessage_ = true;

        // Pin the most recent WARN+ line so it can't scroll away beneath
        // later INFO chatter (e.g. a WiFi failure followed by NTP retries).
        if (level == LogLevel::WARNING || level == LogLevel::ERROR ||
            level == LogLevel::CRITICAL) {
            strncpy(pinnedMessage_, message, sizeof(pinnedMessage_) - 1);
            pinnedMessage_[sizeof(pinnedMessage_) - 1] = '\0';
            pinnedLevel_ = level;
            hasPinnedMessage_ = true;
        }

        // Once the log area is full, scroll its contents up by one line
        // instead of drawing past the bottom of the screen — otherwise later
        // boot messages land off-screen and are simply never seen.
        if (lineY_ + lineHeight_ > lcd_->height()) {
            lcd_->scroll(0, -static_cast<int_fast16_t>(lineHeight_));
            lineY_ -= lineHeight_;
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

    if (hasStatusMessage_)
        drawStatusLine();
    if (hasPinnedMessage_)
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

void BootScreen::drawProgressBar() {
    const uint8_t percent = coreState_.bootProgressPercent > 100 ? 100 : coreState_.bootProgressPercent;
    const int16_t barX = kBarMarginX;
    const int16_t barW = lcd_->width() - 2 * kBarMarginX;

    lcd_->fillSmoothRoundRect(barX, kBarY, barW, kBarH, Theme::kRadiusSm, Theme::kSurface2);

    const int16_t fillW = static_cast<int16_t>((static_cast<int32_t>(barW) * percent) / 100);
    if (fillW > 0) {
        lcd_->fillSmoothRoundRect(barX, kBarY, fillW, kBarH, Theme::kRadiusSm, Colors::kInfo);
    }

    lastDrawnPercent_ = percent;
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
