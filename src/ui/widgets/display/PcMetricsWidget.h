#pragma once
#include <string>

#include "config/AppConfigInterface.h"
#include "MetricWidget.h"
#include "services/pcMetrics/PcMetrics.h"
#include "ui/core/DisplayContext.h"
#include "ui/widgets/base/Widget.h"
#include "ui/widgets/display/ThreadsWidget.h"

class PcMetricsWidget : public Widget {
 public:
    PcMetricsWidget(DisplayContext& context, const WidgetInterface::Dimensions& dims,
                    uint32_t updateIntervalMs, PcMetrics& pcMetrics, AppConfigInterface& config,
                    ApplicationMetrics& systemMetrics);
    void drawStatic() override;
    bool handleTouch(uint16_t x, uint16_t y) override;
    bool needsUpdate() const override;

    void setStaleTimeout(unsigned long timeoutMs);
    unsigned long getStaleTimeout() const;
    bool isDataStale() const;

    void updateData(const PcMetrics& newMetrics) {
        if (newMetrics.is_available) {
            if (!dataWasAvailable_) {
                // First time we have data - force full redraw
                markDataFresh();
                drawStatic();
                dataWasAvailable_ = true;
            } else {
                // Update existing data
                pcMetrics_ = newMetrics;
                markDirty();
            }
        } else {
            // Data unavailable
            markDataStale();
        }
    }

 protected:
    void onDraw(bool forceRedraw) override;

 private:
    DisplayContext& context_;
    PcMetrics& pcMetrics_;
    AppConfigInterface& config_;
    ApplicationMetrics& systemMetrics_;
    unsigned long lastUpdateTimestamp_ = 0;
    unsigned long staleTimeoutMs_ = 5000;
    bool wasFreshData_ = false;
    bool dataWasAvailable_ = false;
    bool isStaticDrawn_ = false;

    std::unique_ptr<MetricWidget> cpuLoadWidget_;
    std::unique_ptr<MetricWidget> cpuTemperatureWidget_;
    std::unique_ptr<MetricWidget> cpuPowerWidget_;
    std::unique_ptr<MetricWidget> cpuFanWidget_;

    std::unique_ptr<MetricWidget> gpuLoadWidget_;
    std::unique_ptr<MetricWidget> gpuTemperatureWidget_;
    std::unique_ptr<MetricWidget> gpu3dWidget_;
    std::unique_ptr<MetricWidget> gpuComputeWidget_;
    std::unique_ptr<MetricWidget> gpuPowerWidget_;
    std::unique_ptr<MetricWidget> gpuMemoryWidget_;
    std::unique_ptr<MetricWidget> gpuFanWidget_;

    std::unique_ptr<MetricWidget> fanWidget1_;
    std::unique_ptr<MetricWidget> fanWidget2_;

    std::unique_ptr<MetricWidget> memoryLoadWidget_;

    void drawNoDataMessage();
    void drawDynamicData();
    void clearAllWidgets();
    void restoreStaticDisplay();
    bool hasFreshData() const;
    void showStaleIndicator();
    void markDataFresh() { wasFreshData_ = true; }
    void markDataStale() {
        wasFreshData_ = false;
        dataWasAvailable_ = false;
    }
};