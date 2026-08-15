#include "ApplicationMetrics.h"

#include <algorithm>

void ApplicationMetrics::setPcMetricsJsonParseTime(uint32_t timeMs) {
    pcMetricsJsonParseTime_ = timeMs;
}

uint32_t ApplicationMetrics::getPcMetricsJsonParseTime() const {
    return pcMetricsJsonParseTime_;
}

void ApplicationMetrics::addScreenDrawTimeUs(uint32_t timeUs) {
    screenDrawTimesUs_[screenDrawIndex_] = timeUs;
    screenDrawIndex_ = (screenDrawIndex_ + 1) % kDrawTimesCapacity;
    if (screenDrawCount_ < kDrawTimesCapacity) {
        screenDrawCount_++;
    }
}

const std::array<uint32_t, ApplicationMetrics::kDrawTimesCapacity>&
ApplicationMetrics::getScreenDrawTimesUs() const {
    return screenDrawTimesUs_;
}

float ApplicationMetrics::getAverageScreenDrawTimeUs() const {
    if (screenDrawCount_ == 0) {
        return 0.0f;
    }

    uint64_t sum = 0;
    size_t start = (screenDrawIndex_ + kDrawTimesCapacity - screenDrawCount_) % kDrawTimesCapacity;
    for (size_t i = 0; i < screenDrawCount_; ++i) {
        sum += screenDrawTimesUs_[(start + i) % kDrawTimesCapacity];
    }
    return static_cast<float>(sum) / static_cast<float>(screenDrawCount_);
}

uint32_t ApplicationMetrics::getMaxScreenDrawTimeUs() const {
    if (screenDrawCount_ == 0) {
        return 0;
    }

    uint32_t maxUs = 0;
    size_t start = (screenDrawIndex_ + kDrawTimesCapacity - screenDrawCount_) % kDrawTimesCapacity;
    for (size_t i = 0; i < screenDrawCount_; ++i) {
        maxUs = std::max(maxUs, screenDrawTimesUs_[(start + i) % kDrawTimesCapacity]);
    }
    return maxUs;
}

uint32_t ApplicationMetrics::getP95ScreenDrawTimeUs() const {
    if (screenDrawCount_ == 0) {
        return 0;
    }

    // The ring buffer only holds kDrawTimesCapacity (30) samples, so a copy
    // + sort on demand (web polling only, never per-frame) is cheap.
    std::array<uint32_t, kDrawTimesCapacity> sorted{};
    size_t start = (screenDrawIndex_ + kDrawTimesCapacity - screenDrawCount_) % kDrawTimesCapacity;
    for (size_t i = 0; i < screenDrawCount_; ++i) {
        sorted[i] = screenDrawTimesUs_[(start + i) % kDrawTimesCapacity];
    }
    std::sort(sorted.begin(), sorted.begin() + screenDrawCount_);

    size_t idx = (screenDrawCount_ * 95) / 100;
    if (idx >= screenDrawCount_) {
        idx = screenDrawCount_ - 1;
    }
    return sorted[idx];
}

size_t ApplicationMetrics::getScreenDrawCount() const {
    return screenDrawCount_;
}

size_t ApplicationMetrics::getScreenDrawStartIndex() const {
    return (screenDrawIndex_ + kDrawTimesCapacity - screenDrawCount_) % kDrawTimesCapacity;
}

void ApplicationMetrics::setThreadsBarDrawTimeUs(uint32_t timeUs) {
    threadsBarDrawTimeUs_ = timeUs;
}

uint32_t ApplicationMetrics::getThreadsBarDrawTimeUs() const {
    return threadsBarDrawTimeUs_;
}

void ApplicationMetrics::getFormattedUptime(char* buf, size_t size) const {
    unsigned long uptimeMs = millis();
    unsigned long seconds = uptimeMs / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;

    // HH:MM:SS only fits 2-digit hours. Past 99 h, switch to "Dd HH:MM" so the
    // string never grows past its fixed shape and callers with small
    // fixed-width buffers (e.g. UptimeWidget) can't get silently truncated
    // into garbage.
    if (hours < 100) {
        snprintf(buf, size, "%02lu:%02lu:%02lu", hours, minutes % 60, seconds % 60);
    } else {
        unsigned long days = hours / 24;
        snprintf(buf, size, "%lud %02lu:%02lu", days, hours % 24, minutes % 60);
    }
}

void ApplicationMetrics::addThreadWidgetFrameTime() {
    threadWidgetFrameCount_++;

    uint32_t currentTime = millis();
    if (!threadWidgetFpsSeeded_) {
        threadWidgetFpsSeeded_ = true;
        threadWidgetFrameCount_ = 0;
        threadWidgetLastFpsTime_ = currentTime;
        return;
    }

    if (currentTime - threadWidgetLastFpsTime_ >= 1000) {
        uint32_t elapsed = currentTime - threadWidgetLastFpsTime_;
        threadWidgetCurrentFps_ =
            elapsed > 0 ? (threadWidgetFrameCount_ * 1000.0f) / elapsed : 0.0f;
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
