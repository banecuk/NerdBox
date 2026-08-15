#pragma once

#include "config/LgfxConfig.h"
#include "ScreenInterface.h"
#include "utils/ScreenLogQueue.h"

class BootScreen : public ScreenInterface {
 public:
    explicit BootScreen(ScreenLogQueue& screenLogQueue, LGFX* lcd);
    ~BootScreen() override = default;

    void initialize();
    void onEnter() override;
    void onExit() override;
    void draw() override;

 private:
    // Top of the scrollable log area (below the title). Fixed so the clip
    // rect set in onEnter() matches where lines actually get drawn.
    static constexpr uint16_t kLogAreaY = 28;

    ScreenLogQueue& screenLogQueue_;
    LGFX* lcd_;

    uint16_t lineY_ = 28;       // y pixel of the next log line
    uint16_t lineHeight_ = 16;  // measured from NotoSansDisplay12 in onEnter()
};