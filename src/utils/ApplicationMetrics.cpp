#include "ApplicationMetrics.h"

ApplicationMetrics::ApplicationMetrics() 
    : pcMetricsJsonParseTime_(0), 
      screenDrawIndex_(0), 
      screenDrawCount_(0) {
    // Initialize array with zeros
    screenDrawTimes_.fill(0);
}

void ApplicationMetrics::setPcMetricsJsonParseTime(uint32_t timeMs) {
    pcMetricsJsonParseTime_ = timeMs;
}

uint32_t ApplicationMetrics::getPcMetricsJsonParseTime() const {
    return pcMetricsJsonParseTime_;
}

void ApplicationMetrics::addScreenDrawTime(uint32_t timeMs) {
    // Store the new time at the current index
    screenDrawTimes_[screenDrawIndex_] = timeMs;
    
    // Advance the index (wrap around if at the end)
    screenDrawIndex_ = (screenDrawIndex_ + 1) % Config::Metrics::kMaxScreenDrawTimes;
    
    // Increment count until the buffer is full
    if (screenDrawCount_ < Config::Metrics::kMaxScreenDrawTimes) {
        screenDrawCount_++;
    }
}

const std::array<uint32_t, Config::Metrics::kMaxScreenDrawTimes>& ApplicationMetrics::getScreenDrawTimes() const {
    return screenDrawTimes_;
}

float ApplicationMetrics::getAverageScreenDrawTime() const {
    // Only compute average if the buffer is full to avoid skew from unset values
    if (screenDrawCount_ < Config::Metrics::kMaxScreenDrawTimes) {
        return 0.0f; // Return 0.0f if not enough data
    }

    uint32_t sum = 0;
    for (uint32_t time : screenDrawTimes_) {
        sum += time;
    }
    return static_cast<float>(sum) / Config::Metrics::kMaxScreenDrawTimes;
}