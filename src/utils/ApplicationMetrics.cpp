#include "ApplicationMetrics.h"

ApplicationMetrics::ApplicationMetrics(AppConfigInterface& config) : config_(config) {}

void ApplicationMetrics::setPcMetricsJsonParseTime(uint32_t timeMs) {
    pcMetricsJsonParseTime_ = timeMs;
}

uint32_t ApplicationMetrics::getPcMetricsJsonParseTime() const {
    return pcMetricsJsonParseTime_;
}

void ApplicationMetrics::addScreenDrawTime(uint32_t timeMs) {
    screenDrawTimes_[screenDrawIndex_] = timeMs;
    screenDrawIndex_ = (screenDrawIndex_ + 1) % kDrawTimesCapacity;
    if (screenDrawCount_ < kDrawTimesCapacity) {
        screenDrawCount_++;
    }
}

const std::array<uint32_t, ApplicationMetrics::kDrawTimesCapacity>&
ApplicationMetrics::getScreenDrawTimes() const {
    return screenDrawTimes_;
}

float ApplicationMetrics::getAverageScreenDrawTime() const {
    if (screenDrawCount_ == 0) {
        return 0.0f;
    }

    uint64_t sum = 0;
    size_t start = (screenDrawIndex_ + kDrawTimesCapacity - screenDrawCount_) % kDrawTimesCapacity;
    for (size_t i = 0; i < screenDrawCount_; ++i) {
        sum += screenDrawTimes_[(start + i) % kDrawTimesCapacity];
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

void ApplicationMetrics::addThreadWidgetFrameTime(uint32_t timeMs) {
    threadWidgetFrameCount_++;

    uint32_t currentTime = millis();
    if (currentTime - threadWidgetLastFpsTime_ >= 1000) {
        uint32_t elapsed = currentTime - threadWidgetLastFpsTime_;
        threadWidgetCurrentFps_ = elapsed > 0
                                       ? (threadWidgetFrameCount_ * 1000.0f) / elapsed
                                       : 0.0f;
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
