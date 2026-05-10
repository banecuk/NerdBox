#pragma once

#include <Arduino.h>

#include <array>

#include "config/AppConfigInterface.h"
#include "config/AppConfig.h"

class ApplicationMetrics {
 public:
    ApplicationMetrics(AppConfigInterface& config);

    // JSON parse time methods
    void setPcMetricsJsonParseTime(uint32_t timeMs);
    uint32_t getPcMetricsJsonParseTime() const;

    // Screen draw time methods
    void addScreenDrawTime(uint32_t timeMs);
    const std::array<uint32_t, AppConfig::internal::MetricsImpl::kMaxScreenDrawTimes>&
    getScreenDrawTimes() const;
    float getAverageScreenDrawTime() const;
    size_t getScreenDrawCount() const;

    // Uptime method
    String getFormattedUptime() const;

    // Thread widget FPS methods
    void addThreadWidgetFrameTime(uint32_t timeMs);
    float getThreadWidgetFPS() const;
    size_t getThreadWidgetFrameCount() const;

 private:
    static constexpr size_t kDrawTimesCapacity =
        AppConfig::internal::MetricsImpl::kMaxScreenDrawTimes;

    AppConfigInterface& config_;

    uint32_t pcMetricsJsonParseTime_ = 0;
    std::array<uint32_t, kDrawTimesCapacity> screenDrawTimes_{};
    size_t screenDrawIndex_ = 0;
    size_t screenDrawCount_ = 0;

    // FPS counter
    uint32_t threadWidgetFrameCount_ = 0;
    uint32_t threadWidgetLastFpsTime_ = 0;
    float threadWidgetCurrentFps_ = 0.0f;
};