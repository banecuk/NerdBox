#pragma once

#include <memory>
#include <vector>

#include "config/LgfxConfig.h"
#include "ui/core/DisplayContext.h"
#include "ui/widgets/base/WidgetInterface.h"
#include "utils/ApplicationMetrics.h"
#include "utils/logging/LoggerInterface.h"

class WidgetManager {
 public:
    WidgetManager(DisplayContext& context, ApplicationMetrics& systemMetrics);
    ~WidgetManager();

    // label must outlive this WidgetManager — pass a string literal from the
    // call site (every current caller does); it is published by pointer into
    // ApplicationMetrics' widget-stats snapshot, not copied.
    void addWidget(std::unique_ptr<WidgetInterface> widget, const char* label);
    void initializeWidgets();
    bool handleTouch(uint16_t x, uint16_t y);
    void cleanupWidgets();
    size_t getWidgetCount() const { return widgetCache_.size(); }

    // Dirty management
    void markAllDirty();

    // Two-phase draw, both halves in the same tick:
    //
    // evaluateDirtyWidgets() — cheap pre-lock pass. Records each entry's
    //   chrome/value-dirty verdict and returns true if there is any work.
    //   Call it before acquiring the display lock so the semaphore take/give
    //   is skipped entirely on idle frames. NOT a pure query: it writes the
    //   per-entry verdicts that updateDirtyWidgets() consumes.
    //
    // updateDirtyWidgets() — paints from those recorded verdicts without
    //   re-deriving them. Must follow an evaluateDirtyWidgets() call in the
    //   same tick; calling it without one logs a warning and paints nothing
    //   rather than painting from stale verdicts (dirtyEvaluated_ guard).
    //
    // BaseWidgetScreen::draw() is the one caller of both.
    bool evaluateDirtyWidgets();
    void updateDirtyWidgets();

 private:
    // Cache structure to avoid repeated getDimensions() calls
    struct WidgetCacheEntry {
        std::unique_ptr<WidgetInterface> widget;
        const char* label = "widget";
        WidgetInterface::Dimensions cachedDims;
        bool isDirty = true;  // Track widget-level dirtiness
        uint32_t lastUpdateTime = 0;

        // Verdict recorded by evaluateDirtyWidgets(), consumed by
        // updateDirtyWidgets() so it doesn't re-derive the same answers.
        bool cachedSkip = true;
        bool cachedChromeDirty = false;
        bool cachedValueDirty = false;

        // Cumulative draw() timing — see 07-performance.md P1-22. Boot-to-
        // (last screen transition) totals, not a ring: at 60 fps the ratios
        // between widgets converge within seconds, so no history is needed.
        uint32_t drawCalls = 0;
        uint64_t drawTotalUs = 0;
    };

    DisplayContext& context_;
    LoggerInterface& logger_;
    LGFX* lcd_;
    ApplicationMetrics& systemMetrics_;
    std::vector<WidgetCacheEntry> widgetCache_;
    bool isInitialized_ = false;

    bool allDirty_ = false;
    uint32_t lastStatsLogTime_ = 0;

    // Set by evaluateDirtyWidgets(), consumed by updateDirtyWidgets() at the
    // end of its pass to detect P1-20's exact failure shape in general: work
    // was found, but nothing actually got drawn.
    bool lastEvaluateFoundWork_ = false;

    // Set by evaluateDirtyWidgets(), checked and cleared by
    // updateDirtyWidgets() — enforces the evaluate-then-update ordering
    // instead of leaving it to the caller.
    bool dirtyEvaluated_ = false;

    // Publishes the top few widgets by cumulative draw time into
    // systemMetrics_, sorted descending — called after every
    // updateDirtyWidgets() pass that did any work.
    void publishWidgetStats() const;
};