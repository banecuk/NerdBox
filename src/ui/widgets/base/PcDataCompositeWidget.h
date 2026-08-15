#pragma once

#include "services/pcMetrics/PcMetrics.h"
#include "ui/widgets/base/Widget.h"
#include "ui/widgets/display/MetricWidget.h"
#include "utils/DataFreshnessGuard.h"

// Base for widgets that render a grid/strip of MetricWidget children driven
// by PcMetrics — PcMetricsWidget (CPU/GPU/RAM tile grid) and DiskBandWidget
// (per-drive strip). Both are "container widget that owns child
// MetricWidgets, rebuilds them when the data shape changes, and blanks
// itself when data goes stale"; this holds the freshness transition, the
// timestamp gate, and initAndDrawWidget() — previously hand-duplicated in
// both (see docs-local/02-architecture.md, A5).
class PcDataCompositeWidget : public Widget {
 public:
    void setStaleTimeout(unsigned long timeoutMs) { freshnessGuard_.setTimeout(timeoutMs); }
    bool needsUpdate() const override;

 protected:
    PcDataCompositeWidget(const WidgetInterface::Dimensions& dims, uint32_t updateIntervalMs,
                          PcMetrics& pcMetrics);

    bool hasFreshData() const { return freshnessGuard_.isFresh(); }

    // initialize() + drawStatic() + forceRefresh() on a freshly-(re)built
    // child MetricWidget tile — identical across every composite widget.
    // initialize() draws static chrome on first call only (no-op on repeat
    // calls once the tile is already initialized), so the explicit
    // drawStatic() is what actually repaints chrome on every subsequent
    // stale->fresh transition; forceRefresh() resets the tile's cached
    // layout state and performs the initial value draw immediately.
    void initAndDrawWidget(MetricWidget& widget);

    // ---- Hooks a derived class must implement ----

    // hasFreshData() was true and static chrome needs (re)drawing: (re)build
    // any data-shaped child widgets (typically via ensureChildWidgetsCreated()),
    // initialize + draw every child, and paint any static chrome that never
    // changes with the data (e.g. a chevron).
    virtual void drawFreshStatic() = 0;

    // Redraw every child widget's current value. Called whenever fresh data
    // is available and a redraw is due.
    virtual void drawDynamicData() = 0;

    // Clear every child widget and any extra pixels drawn outside them
    // (activity lines, chevron, etc). isStaticDrawn_ and the shared
    // lastUpdateTimestamp_ gate below are reset by the caller, not by this
    // hook.
    virtual void clearChildren() = 0;

    // Rebuild data-dependent child widgets when the data shape changes (e.g.
    // a drive appears/disappears, a fan count changes). Called once per new
    // data arrival, before drawDynamicData(). No-op default for a widget
    // whose children are all fixed at construction time.
    virtual void ensureChildWidgetsCreated() {}

    // Placeholder shown instead of children when there's no fresh data.
    // No-op by default (DiskBandWidget shows nothing; PcMetricsWidget shows
    // "No Data").
    virtual void drawNoDataMessage() {}

    PcMetrics& pcMetrics_;

    // Shared by both the "does the data shape need re-checking" gate and
    // needsUpdate()'s "is there new data to draw" check — see the comment on
    // needsUpdate()'s definition in the .cpp for why one field for both is
    // safe.
    unsigned long lastUpdateTimestamp_ = 0;

 private:
    DataFreshnessGuard freshnessGuard_;
    bool wasFreshData_ = false;

    void onDrawStatic() override final;
    void onDraw(bool forceRedraw) override final;
};
