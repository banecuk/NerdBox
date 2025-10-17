#include "ApplicationMetrics.h"

ApplicationMetrics::ApplicationMetrics(AppConfigInterface& config) 
    : pcMetricsJsonParseTime_(0),
      screenDrawCapacity_(static_cast<size_t>(config_.getMetricsMaxScreenDrawTimes())),
      screenDrawIndex_(0), 
      screenDrawCount_(0),
      screenDrawTimes_(),
      config_(config) {
    // Initialize vector with zeros sized from config
    if (screenDrawCapacity_ == 0) {
        screenDrawCapacity_ = 1; // avoid zero-size vector if config returns 0
    }
    screenDrawTimes_.assign(screenDrawCapacity_, 0);
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
    screenDrawIndex_ = (screenDrawIndex_ + 1) % screenDrawCapacity_;

    // Increment count until the buffer is full
    if (screenDrawCount_ < screenDrawCapacity_) {
        screenDrawCount_++;
    }
}

const std::vector<uint32_t>& ApplicationMetrics::getScreenDrawTimes() const {
    return screenDrawTimes_;
}

float ApplicationMetrics::getAverageScreenDrawTime() const {
    if (screenDrawCount_ == 0) {
        return 0.0f;
    }

    // Sum only the valid entries (respecting circular buffer)
    uint64_t sum = 0;
    size_t start = (screenDrawIndex_ + screenDrawCapacity_ - screenDrawCount_) % screenDrawCapacity_;
    for (size_t i = 0; i < screenDrawCount_; ++i) {
        size_t idx = (start + i) % screenDrawCapacity_;
        sum += screenDrawTimes_[idx];
    }
    return static_cast<float>(sum) / static_cast<float>(screenDrawCount_);
}

size_t ApplicationMetrics::getScreenDrawCount() const {
    return screenDrawCount_;
}

String ApplicationMetrics::getFormattedUptime() const {
    char buffer[20];
    unsigned long uptimeMs = millis();
    unsigned long seconds = uptimeMs / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;
    snprintf(buffer, sizeof(buffer), "%02lu:%02lu:%02lu", hours, minutes % 60, seconds % 60);
    return String(buffer);
}