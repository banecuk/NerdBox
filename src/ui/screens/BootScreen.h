#pragma once

#include "config/LgfxConfig.h"
#include "core/state/SystemState.h"
#include "ui/screens/base/ScreenInterface.h"
#include "utils/logging/LogTypes.h"
#include "utils/logging/ScreenLogQueue.h"

// First-boot splash: centred wordmark + build string, a determinate progress
// bar driven by InitializationStateMachine's phases (via
// SystemState::CoreState::bootProgressPercent), a single rolling status line
// coloured by the message's LogLevel, a pinned line for the most recent
// WARN+ message (so a WiFi/NTP failure can't scroll away unnoticed beneath
// later INFO chatter), and the original scrolling log — demoted to a small,
// dim, bottom two-thirds strip instead of the whole screen. See
// docs-local/03-visual-ux.md V10.
class BootScreen : public ScreenInterface {
 public:
    BootScreen(ScreenLogQueue& screenLogQueue, const SystemState::CoreState& coreState, LGFX* lcd);
    ~BootScreen() override = default;

    void onEnter() override;
    void onExit() override;
    void draw() override;

 private:
    // Header layout — wordmark/build line/progress bar/status/pinned line,
    // all above the demoted scrolling log.
    static constexpr uint16_t kWordmarkY = 2;
    static constexpr uint16_t kBuildLineY = 28;
    static constexpr uint16_t kBarY = 46;
    static constexpr uint16_t kBarH = 8;
    static constexpr uint16_t kBarMarginX = 60;
    static constexpr uint16_t kStatusLineY = 62;
    static constexpr uint16_t kPinnedLineY = 80;
    // Top of the scrollable log area — roughly the bottom two-thirds of a
    // 320px-tall screen. Fixed so the clip rect set in onEnter() matches
    // where lines actually get drawn.
    static constexpr uint16_t kLogAreaY = 100;

    ScreenLogQueue& screenLogQueue_;
    const SystemState::CoreState& coreState_;
    LGFX* lcd_;

    uint16_t lineY_ = kLogAreaY;  // y pixel of the next scrolling log line
    uint16_t lineHeight_ = 16;    // measured from NotoSansDisplay12 in onEnter()

    // Sentinel so the very first draw() always paints the bar even though
    // bootProgressPercent may already be 0 by then.
    uint8_t lastDrawnPercent_ = 0xFF;

    bool hasStatusMessage_ = false;
    char statusMessage_[200] = {0};
    LogLevel statusLevel_ = LogLevel::INFO;

    // Most recent WARNING/ERROR/CRITICAL message — latched here instead of
    // just scrolling past with the rest of the log.
    bool hasPinnedMessage_ = false;
    char pinnedMessage_[200] = {0};
    LogLevel pinnedLevel_ = LogLevel::WARNING;

    void drawWordmark();
    void drawProgressBar();
    void drawStatusLine();
    void drawPinnedLine();
};
