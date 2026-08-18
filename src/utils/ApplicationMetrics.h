#pragma once

#include <Arduino.h>

#include <array>

class ApplicationMetrics {
 public:
    // Capacity of the circular screen-draw-time buffer.
    // Defined here (not pulled from AppConfig) so utils/ remains independent
    // of config/AppConfig.h.  AppSettings still exposes the same value via
    // metricsMaxScreenDrawTimes for runtime queries. Kept in sync with
    // AppConfig::internal::MetricsImpl::kMaxScreenDrawTimes by a static_assert
    // in ApplicationComponents.h, the one place both headers are included.
    static constexpr size_t kDrawTimesCapacity = 30;

    ApplicationMetrics() = default;

    // JSON parse time methods
    void setPcMetricsJsonParseTime(uint32_t timeMs);
    uint32_t getPcMetricsJsonParseTime() const;

    // SSE stream path's counterpart to the above — the default data path
    // (PcMetricsStreamService::handleEvent()) never called into
    // setPcMetricsJsonParseTime(), leaving parse_ms permanently 0 while
    // streaming. Reported separately (µs, not ms — see the draw-time comment
    // below) so the polling fallback's parse_ms stays meaningful on its own.
    void setPcMetricsStreamParseTimeUs(uint32_t timeUs);
    uint32_t getPcMetricsStreamParseTimeUs() const;

    // Screen draw time methods — recorded in microseconds. A 16 ms frame
    // budget measured in whole milliseconds quantizes almost every frame to
    // 0 or 1, making the reported average meaningless; micros() gives real
    // resolution. Callers format back to ms (with decimals) for display.
    void addScreenDrawTimeUs(uint32_t timeUs);
    const std::array<uint32_t, kDrawTimesCapacity>& getScreenDrawTimesUs() const;
    float getAverageScreenDrawTimeUs() const;
    // Max/p95 alongside the mean — the mean alone hides an occasional slow
    // frame (e.g. the 100 ms grid tick) inside a run of cheap ones.
    uint32_t getMaxScreenDrawTimeUs() const;
    uint32_t getP95ScreenDrawTimeUs() const;
    size_t getScreenDrawCount() const;

    // Index into getScreenDrawTimesUs() of the oldest sample still held —
    // callers that want to print/emit the buffer in chronological order
    // should start here and wrap modulo kDrawTimesCapacity, rather than
    // reading slot order.
    size_t getScreenDrawStartIndex() const;

    // Coarse per-phase draw timing, microseconds — only ThreadsWidget is
    // wired up (see its onDraw, gated on DEBUG_MODE); the setter is cheap
    // enough to leave always-compiled, so a release build just never calls
    // it and the value stays 0.
    void setThreadsBarDrawTimeUs(uint32_t timeUs);
    uint32_t getThreadsBarDrawTimeUs() const;

    // Uptime method — writes directly into caller's buffer; zero heap allocation.
    void getFormattedUptime(char* buf, size_t size) const;

    // Thread widget FPS methods
    void addThreadWidgetFrameTime();
    float getThreadWidgetFPS() const;
    size_t getThreadWidgetFrameCount() const;

 private:
    uint32_t pcMetricsJsonParseTime_ = 0;
    uint32_t pcMetricsStreamParseTimeUs_ = 0;
    std::array<uint32_t, kDrawTimesCapacity> screenDrawTimesUs_{};
    size_t screenDrawIndex_ = 0;
    size_t screenDrawCount_ = 0;

    uint32_t threadsBarDrawTimeUs_ = 0;

    // FPS counter
    uint32_t threadWidgetFrameCount_ = 0;
    uint32_t threadWidgetLastFpsTime_ = 0;
    float threadWidgetCurrentFps_ = 0.0f;
    // False until the first addThreadWidgetFrameTime() call seeds
    // threadWidgetLastFpsTime_ — without this, the first elapsed-window
    // check measures against millis()==0 instead of "now", reporting a
    // nonsense FPS for the whole uptime-so-far (see B24).
    bool threadWidgetFpsSeeded_ = false;
};