#include "ThreadsWidget.h"

#include <algorithm>

#include "ui/core/UiText.h"
#include "ui/resources/FontRegistry.h"

// DEBUG_MODE is defined by the build environment (-DDEBUG_MODE=1 or =0); see
// Logger.cpp for the same pattern. Guards the drawBars() phase timer so
// release builds pay nothing for it.
#ifndef DEBUG_MODE
    #define DEBUG_MODE 0
#endif

ThreadsWidget::ThreadsWidget(const WidgetInterface::Dimensions& dims, uint32_t updateIntervalMs,
                             PcMetrics& pcMetrics, const AppSettings& config,
                             ApplicationMetrics& systemMetrics, TapAction tap)
    : Widget(dims, updateIntervalMs),
      pcMetrics_(pcMetrics),
      config_(config),
      systemMetrics_(systemMetrics),
      tap_(std::move(tap)),
      freshness_(pcMetrics.freshness) {}

void ThreadsWidget::initialize(DisplayContext& context) {
    Widget::initialize(context);

    // Set update interval to the faster threads refresh rate for high FPS animation
    setUpdateInterval(config_.hardwareMonitorThreadsRefreshMs);

    // Initialize the value smoother with current data if available. Recording
    // the verdict here (rather than leaving it to the first onDraw()) keeps a
    // widget that starts fresh from treating its first draw as a stale→fresh
    // recovery.
    freshness_.poll(freshness_.isFreshNow() && ensureLayoutInitialized());
    if (freshness_.fresh()) {
        updateSmoothedValues();
    }
}

bool ThreadsWidget::ensureLayoutInitialized() {
    if (coreCount_ != 0) {
        return true;
    }

    const uint8_t detected = pcMetrics_.cpu_core_count;
    if (detected == 0) {
        return false;  // No CoreLoads payload has arrived yet
    }

    coreCount_ = detected;
    barWidth_ = dimensions_.width / coreCount_;
    remainder_ = dimensions_.width % coreCount_;
    previousBarHeights_.assign(coreCount_, 0);
    previousColors_.assign(coreCount_, 0);
    smoothedThreadLoads_.assign(coreCount_, 0);
    stagedTargets_.assign(coreCount_, 0);
    valueSmoother_ =
        std::make_unique<ValueSmoother>(coreCount_, config_.hardwareMonitorThreadsUpwardSmoothing,
                                        config_.hardwareMonitorThreadsDownwardSmoothing);

    // P/E-core colour split — coreCount_ is the *reported* thread count, so a
    // PC reporting fewer threads than kThreadsPerformanceCount (different
    // machine, HT off) falls back to today's uniform look automatically via
    // CpuTopology::hasSplit().
    const uint8_t perf = config_.hardwareMonitorThreadsCoreClassTint
                              ? config_.hardwareMonitorThreadsPerformanceCount
                              : uint8_t(0);
    isEcore_.assign(coreCount_, 0);
    for (uint8_t i = 0; i < coreCount_; ++i) {
        isEcore_[i] = (CpuTopology::classOf(i, perf, coreCount_) == CpuTopology::CoreClass::Efficiency);
    }

    stagger_.configure(coreCount_, config_.hardwareMonitorThreadsStaggerFraction,
                       config_.hardwareMonitorThreadsStaggerFallbackPeriodMs,
                       config_.hardwareMonitorThreadsStaggerMinPeriodMs,
                       config_.hardwareMonitorThreadsStaggerMaxPeriodMs);
    stagger_.seed(micros());
    // Boot: bars should fill immediately, not ripple in from stale zeros.
    stagger_.releaseAll(millis());
    return true;
}

void ThreadsWidget::onDrawStatic() {
    // Clear the widget area once
    getLcd()->fillRect(dimensions_.x, dimensions_.y, dimensions_.width, dimensions_.height,
                       TFT_BLACK);

    // Reset cached state so the next draw treats every bar as changed
    std::fill(previousBarHeights_.begin(), previousBarHeights_.end(), 0);
    std::fill(previousColors_.begin(), previousColors_.end(), 0);
}

void ThreadsWidget::onDraw(bool forceRedraw) {
    if (!getLcd())
        return;

    const auto edge = freshness_.poll(freshness_.isFreshNow() && ensureLayoutInitialized());
    const bool fresh = freshness_.fresh();

    if (edge == FreshnessTransition::Edge::BecameFresh) {
        // Coming back from stale: reset cached bar state so the next
        // drawBars() treats every bar as changed instead of diffing against
        // pre-outage heights/colors left over from before the "No Data"
        // message was shown.
        onDrawStatic();
        // Bars refill instantly on recovery — no ripple-in.
        stagger_.releaseAll(millis());
    }

    if (fresh) {
        updateSmoothedValues();
        systemMetrics_.addThreadWidgetFrameTime();
#if DEBUG_MODE
        const uint32_t drawStartUs = micros();
        drawBars();
        systemMetrics_.setThreadsBarDrawTimeUs(micros() - drawStartUs);
#else
        drawBars();
#endif
    } else if (edge == FreshnessTransition::Edge::BecameStale || forceRedraw) {
        // Just went stale (or a forced repaint while stale): replace the
        // frozen bars with an explicit message instead of leaving them
        // looking like live (if dim) data.
        drawNoDataMessage();
    }
}

void ThreadsWidget::updateSmoothedValues() {
    // Called every screen tick (kThreadsRefreshMs), not just when a new fetch
    // lands (kRefreshMs). This is intentional: the raw target stays constant
    // between fetches, so repeated calls just keep nudging the smoothed value
    // toward it, and the kThreadsUpward/DownwardSmoothing alphas are tuned for
    // this per-tick cadence to produce a fast-attack / slow-decay VU-meter
    // animation. Gating this on pcMetrics_.freshness.lastUpdateMs() would turn
    // that smooth animation into a hard step every fetch instead.
    //
    // Only the *target latch* below is gated on the publisher's timestamp:
    // stagger_ decides, per bar, when a newly-arrived raw value is allowed to
    // become that bar's new target, spreading 28 simultaneous target jumps
    // across a window instead of latching them all on the same tick. A bar
    // not yet released just keeps chasing its previous target — the smoother
    // itself still runs every tick, unaffected.
    const uint32_t now = millis();
    if (config_.hardwareMonitorThreadsStaggerEnabled) {
        stagger_.tick(now, pcMetrics_.freshness.lastUpdateMs());
    } else {
        stagger_.releaseAll(now);
    }
    for (uint8_t i = 0; i < coreCount_; ++i) {
        if (stagger_.isReleased(i, now)) {
            stagedTargets_[i] = pcMetrics_.cpu_thread_load[i];
        }
    }

    valueSmoother_->update(stagedTargets_.data(), coreCount_);
    valueSmoother_->getSmoothedValues(smoothedThreadLoads_.data(), coreCount_);
}

void ThreadsWidget::drawNoDataMessage() {
    LGFX* lcd = getLcd();
    lcd->fillRect(dimensions_.x, dimensions_.y, dimensions_.width, dimensions_.height, TFT_BLACK);

    Fonts::loadLabel(lcd);
    lcd->setTextColor(TFT_DARKGREY, TFT_BLACK);
    lcd->setTextDatum(MC_DATUM);
    lcd->drawString(UiText::kNoData, dimensions_.x + dimensions_.width / 2,
                    dimensions_.y + dimensions_.height / 2);
    Fonts::unload(lcd);
}

uint16_t ThreadsWidget::barLeft(uint8_t i) const {
    // The first remainder_ bars get one extra pixel of pitch so the row
    // exactly fills dimensions_.width regardless of coreCount_ (see N3/V20).
    const uint16_t extraBefore = (i < remainder_) ? i : remainder_;
    return dimensions_.x + i * barWidth_ + extraBefore;
}

void ThreadsWidget::drawBars() {
    const uint16_t maxBarHeight = dimensions_.height - 1;
    LGFX* lcd = getLcd();
    const int coreCount = coreCount_;

    for (int i = 0; i < coreCount; ++i) {
        const uint8_t threadLoad = smoothedThreadLoads_[i];
        uint16_t newHeight = static_cast<uint16_t>(threadLoad * (maxBarHeight - 1) / 100);
        newHeight = min(newHeight, maxBarHeight);
        newHeight = newHeight + 1;  // Ensure minimum visible height

        const uint16_t newColor =
            isEcore_[i] ? getContext().getColors().getColorFromPercentEcore(threadLoad)
                        : getContext().getColors().getColorFromPercent(threadLoad, false);
        const uint16_t oldHeight = previousBarHeights_[i];
        const uint16_t oldColor = previousColors_[i];

        const uint16_t x = barLeft(static_cast<uint8_t>(i));
        const uint16_t w = barLeft(static_cast<uint8_t>(i + 1)) - x - 1;

        if (newHeight == oldHeight && newColor == oldColor) {
            continue;  // Bar unchanged — no pixel writes needed
        }

        if (newColor != oldColor) {
            // Color threshold crossed: repaint the entire bar in the new color,
            // then erase any excess from the old (taller) portion.
            if (newHeight > 0) {
                lcd->fillRect(x, dimensions_.y + maxBarHeight - newHeight, w, newHeight, newColor);
            }
            if (newHeight < oldHeight) {
                // Erase the strip above the new top
                lcd->fillRect(x, dimensions_.y + maxBarHeight - oldHeight, w, oldHeight - newHeight,
                              TFT_BLACK);
            }
        } else if (newHeight > oldHeight) {
            // Bar grew — fill only the new top strip, no erase needed
            const uint16_t delta = newHeight - oldHeight;
            lcd->fillRect(x, dimensions_.y + maxBarHeight - newHeight, w, delta, newColor);
        } else {
            // Bar shrank — erase only the vacated top strip
            const uint16_t delta = oldHeight - newHeight;
            lcd->fillRect(x, dimensions_.y + maxBarHeight - oldHeight, w, delta, TFT_BLACK);
        }

        previousBarHeights_[i] = newHeight;
        previousColors_[i] = newColor;
    }
}

bool ThreadsWidget::needsUpdate() const {
    if (!isInitialized()) {
        return false;
    }

    // Mirrors onDraw()'s "fresh" without mutating layout state here: layout
    // initialization only happens inside ensureLayoutInitialized(), called
    // from the non-const onDraw()/initialize().
    const bool dataFresh = freshness_.isFreshNow();
    const bool fresh = dataFresh && (coreCount_ != 0 || pcMetrics_.cpu_core_count != 0);
    if (fresh != freshness_.fresh()) {
        return true;  // one redraw to switch between bars and "No Data"
    }
    if (!fresh) {
        return false;  // "No Data" already shown; nothing changes while stale
    }
    return Widget::needsUpdate();
}

bool ThreadsWidget::handleTouch(uint16_t /*x*/, uint16_t /*y*/) {
    // WidgetManager has already hit-tested (x, y) against this widget's box.
    return tap_.fire();
}