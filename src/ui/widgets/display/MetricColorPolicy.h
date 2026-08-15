#pragma once

#include <cstdint>

// Pure value→percent policy extracted from MetricWidget::calculateBackgroundColor().
// No LGFX/Colors dependency, so it's host-testable under [env:native] — this
// was previously the one piece of MetricWidget that was pure arithmetic and
// completely untested. The final percent→RGB565 palette lookup itself stays
// in MetricWidget, since it needs a live Colors instance.
namespace MetricColorPolicy {

// Returns a 0-100 "how alarming is this value" percentage: 0 at/below the
// good end of [lowerThreshold, upperThreshold], 100 at/above the bad end,
// linearly interpolated in between. reverseThresholds flips which end is
// "good" (e.g. free disk space, where a low value is the bad one).
uint8_t normalizedPercent(int value, float lowerThreshold, float upperThreshold,
                          bool reverseThresholds);

}  // namespace MetricColorPolicy
