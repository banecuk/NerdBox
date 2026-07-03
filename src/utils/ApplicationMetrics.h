#pragma once

#include <Arduino.h>

#include <array>

class ApplicationMetrics {
 public:
    // Capacity of the circular screen-draw-time buffer.
    // Defined here (not pulled from AppConfig) so utils/ remains independent
    // of config/AppConfig.h.  AppConfigService still exposes the same value
    // via getMetricsMaxScreenDrawTimes() for runtime queries.
    static constexpr size_t kDrawTimesCapacity = 30;

    ApplicationMetrics() = default;

    // JSON parse time methods
    void setPcMetricsJsonParseTime(uint32_t timeMs);
    uint32_t getPcMetricsJsonParseTime() const;

    // Screen draw time methods
    void addScreenDrawTime(uint32_t timeMs);
    const std::array<uint32_t, kDrawTimesCapacity>& getScreenDrawTimes() const;
    float getAverageScreenDrawTime() const;
    size_t getScreenDrawCount() const;

    // Uptime method — writes directly into caller's buffer; zero heap allocation.
    void getFormattedUptime(char* buf, size_t size) const;

    // Thread widget FPS methods
    void addThreadWidgetFrameTime();
    float getThreadWidgetFPS() const;
    size_t getThreadWidgetFrameCount() const;

 private:
    uint32_t pcMetricsJsonParseTime_ = 0;
    std::array<uint32_t, kDrawTimesCapacity> screenDrawTimes_{};
    size_t screenDrawIndex_ = 0;
    size_t screenDrawCount_ = 0;

    // FPS counter
    uint32_t threadWidgetFrameCount_ = 0;
    uint32_t threadWidgetLastFpsTime_ = 0;
    float threadWidgetCurrentFps_ = 0.0f;
};