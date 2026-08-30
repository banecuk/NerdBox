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

    // Per-widget cumulative draw-time breakdown — see 07-performance.md
    // P1-22. Whichever screen's WidgetManager is currently active publishes
    // its top few widgets by total draw time here after every
    // updateDirtyWidgets() pass; each screen owns its own WidgetManager (see
    // BaseWidgetScreen), so this resets on a screen transition rather than
    // accumulating across the device's whole uptime — fine for the intended
    // use (attributing a screen's frame-time regression to a widget), since
    // the ratios between widgets converge within seconds at 60 fps. `label`
    // points at a call site's string literal (static storage), so copying
    // the pointer across the screen/main-loop task boundary is safe; the
    // struct itself is written on the screen task and read on the main-loop
    // task without a mutex, same "scalar fields, no mutex" convention as the
    // rest of this class — a torn read is cosmetic (self-heals next publish),
    // not a crash risk.
    struct WidgetDrawStat {
        const char* label = "";
        uint32_t calls = 0;
        uint64_t totalUs = 0;
    };
    static constexpr size_t kWidgetStatsCapacity = 8;

    void setWidgetDrawStats(const WidgetDrawStat* stats, size_t count);
    const std::array<WidgetDrawStat, kWidgetStatsCapacity>& getWidgetDrawStats() const;
    size_t getWidgetDrawStatsCount() const;

    // Frames where WidgetManager::hasAnyDirtyWidgets() found work but
    // updateDirtyWidgets() went on to draw nothing — P1-20's exact failure
    // shape (a needsUpdate() override that consumes a one-shot signal on the
    // pre-check but returns false again on the actual draw call), tracked
    // generically so a future regression of the same shape shows up here
    // instead of needing to be re-discovered by reading code.
    void incrementNoopDirtyFrames();
    uint32_t getNoopDirtyFrames() const;

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

    std::array<WidgetDrawStat, kWidgetStatsCapacity> widgetDrawStats_{};
    size_t widgetDrawStatsCount_ = 0;
    uint32_t noopDirtyFrames_ = 0;
};