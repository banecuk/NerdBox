#include "ApplicationMetrics.h"

ApplicationMetrics::ApplicationMetrics(AppConfigInterface& config)
    : pcMetricsJsonParseTime_(0),
      screenDrawCapacity_(static_cast<size_t>(config_.getMetricsMaxScreenDrawTimes())),
      screenDrawIndex_(0),
      screenDrawCount_(0),
      screenDrawTimes_(),
      config_(config) {
    if (screenDrawCapacity_ == 0) {
        screenDrawCapacity_ = 1;
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
    screenDrawTimes_[screenDrawIndex_] = timeMs;

    screenDrawIndex_ = (screenDrawIndex_ + 1) % screenDrawCapacity_;

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

    uint64_t sum = 0;
    size_t start =
        (screenDrawIndex_ + screenDrawCapacity_ - screenDrawCount_) % screenDrawCapacity_;
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

// SIMPLE FPS COUNTER - Option 3
void ApplicationMetrics::addThreadWidgetFrameTime(uint32_t timeMs) {
    // Just count that a frame was drawn (ignore the timeMs parameter)
    threadWidgetFrameCount_++;

    // Calculate FPS every second
    uint32_t currentTime = millis();
    if (currentTime - threadWidgetLastFpsTime_ >= 1000) {
        uint32_t elapsed = currentTime - threadWidgetLastFpsTime_;
        if (elapsed > 0) {
            threadWidgetCurrentFps_ = (threadWidgetFrameCount_ * 1000.0f) / elapsed;
        } else {
            threadWidgetCurrentFps_ = 0.0f;
        }
        threadWidgetFrameCount_ = 0;
        threadWidgetLastFpsTime_ = currentTime;
    }
}

float ApplicationMetrics::getThreadWidgetFPS() const {
    return threadWidgetCurrentFps_;
}

size_t ApplicationMetrics::getThreadWidgetFrameCount() const {
    return threadWidgetFrameCount_;
}