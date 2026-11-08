#pragma once

#include <cstdint>

#include "utils/DataFreshnessGuard.h"

// Edge detector over DataFreshnessGuard (A13): replaces the hand-named
// "was fresh" bools (lastAvail_/lastHasData_/wasFresh_) that a dozen widgets
// each paired with their own guard to spot stale→fresh / fresh→stale.
//
//   const auto edge = freshness_.poll(dayCount > 0);
//   if (!freshness_.fresh()) {
//       if (forceRedraw || edge == FreshnessTransition::Edge::BecameStale) drawNoData();
//       return;
//   }
//   const bool repaintAll = forceRedraw || edge == FreshnessTransition::Edge::BecameFresh;
//
// Starts "not fresh", so the first fresh poll reports BecameFresh and a stale
// first poll reports None (pair it with forceRedraw, which the first draw
// always is). Widgets don't need to reset() from onDrawStatic(): every
// drawStatic() call site (WidgetManager, MultiWidget) follows it with a
// forced draw, and forceRedraw already means "repaint everything".
//
// Also the single place to hang V9 ("stale = dimmed in place",
// 03-visual-ux.md) once that lands. Host-tested in
// test/FreshnessTransitionTest.cpp.
class FreshnessTransition {
 public:
    enum class Edge : uint8_t {
        None,
        BecameFresh,
        BecameStale
    };

    explicit FreshnessTransition(const PublishedFlag& published,
                                 unsigned long timeoutMs = DataFreshnessGuard::kDefaultTimeoutMs,
                                 DataFreshnessGuard::ClockFn clock = &millis)
        : guard_(published, timeoutMs, clock) {}

    // Samples the guard (ANDed with `hasContent`, for widgets that also need
    // e.g. a non-zero dayCount before they have anything to draw) and reports
    // whether the verdict flipped since the previous poll.
    Edge poll(bool hasContent = true) {
        const bool now = hasContent && guard_.isFresh();
        const bool was = fresh_;
        fresh_ = now;
        if (now == was) {
            return Edge::None;
        }
        return now ? Edge::BecameFresh : Edge::BecameStale;
    }

    // The verdict from the last poll() — what is currently on screen. Safe
    // to read from a const needsUpdate() (unlike poll()).
    bool fresh() const { return fresh_; }

    // The live guard verdict, without recording it — for side-effect-free
    // checks such as needsUpdate() overrides.
    bool isFreshNow() const { return guard_.isFresh(); }

    // Forget the last verdict: the next fresh poll reports BecameFresh.
    void reset() { fresh_ = false; }

    void setTimeout(unsigned long timeoutMs) { guard_.setTimeout(timeoutMs); }

 private:
    DataFreshnessGuard guard_;
    bool fresh_ = false;
};
