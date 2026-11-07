#include "ActivityDetector.h"

ActivityDetector::ActivityDetector(uint16_t enterLevel, uint16_t exitLevel, uint32_t quietMs,
                                   uint32_t enterHoldMs)
    : enterLevel_(enterLevel),
      exitLevel_(exitLevel),
      quietMs_(quietMs),
      enterHoldMs_(enterHoldMs) {}

void ActivityDetector::tick(uint32_t nowMs, bool fresh, uint16_t cpuValue, uint16_t gpuValue) {
    const bool aboveExit = fresh && (cpuValue >= exitLevel_ || gpuValue >= exitLevel_);
    const bool aboveEnter = fresh && (cpuValue >= enterLevel_ || gpuValue >= enterLevel_);

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
