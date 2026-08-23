#include "ActivityDetector.h"

ActivityDetector::ActivityDetector(uint8_t enterPct, uint8_t exitPct, uint32_t quietMs,
                                   uint32_t enterHoldMs)
    : enterPct_(enterPct), exitPct_(exitPct), quietMs_(quietMs), enterHoldMs_(enterHoldMs) {}

void ActivityDetector::tick(uint32_t nowMs, bool fresh, uint8_t cpuLoad, uint8_t gpuLoad) {
    const bool aboveExit = fresh && (cpuLoad >= exitPct_ || gpuLoad >= exitPct_);
    const bool aboveEnter = fresh && (cpuLoad >= enterPct_ || gpuLoad >= enterPct_);

    if (!active_) {
        if (!aboveExit) {
            holding_ = false;
        } else if (!holding_) {
            if (aboveEnter) {
                holding_ = true;
                aboveSinceMs_ = nowMs;
            }
        } else if (nowMs - aboveSinceMs_ >= enterHoldMs_) {
            active_ = true;
            holding_ = false;
            quieting_ = false;
        }
    } else {
        if (aboveExit) {
            quieting_ = false;
        } else if (!quieting_) {
            quieting_ = true;
            belowSinceMs_ = nowMs;
        } else if (nowMs - belowSinceMs_ >= quietMs_) {
            active_ = false;
            holding_ = false;
            quieting_ = false;
        }
    }
}
