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
                markDataFresh();
                drawStatic();
                dataWasAvailable_ = true;
            } else {
                pcMetrics_ = newMetrics;
                markDirty();
            }
        } else {
            markDataStale();
        }
    }

 protected:
    void onDraw(bool forceRedraw) override;

 private:
    // -----------------------------------------------------------------------
    // Layout constants — all magic numbers live here
    // -----------------------------------------------------------------------

    // Screen width the widget is designed for
    static constexpr uint16_t kScreenWidth = 480;

    // Uniform width of every metric tile
    static constexpr uint16_t kTileWidth = 86;

    // Width of the text label inside each tile (e.g. "CPU", "TMP")
    static constexpr uint8_t kLabelWidth = 26;

    // Narrow label for the system-fan tiles (shorter label text)
    static constexpr uint8_t kFanLabelWidth = 14;

    // Horizontal grid — counted from the right edge so tiles stay aligned
    // regardless of kScreenWidth changes.
    static constexpr uint16_t kColFan = 0;                            // system fans / disk area (left)
    static constexpr uint16_t kCol5   = kScreenWidth - kTileWidth * 5;
    static constexpr uint16_t kCol6   = kScreenWidth - kTileWidth * 4;
    static constexpr uint16_t kCol7   = kScreenWidth - kTileWidth * 3;
    static constexpr uint16_t kCol8   = kScreenWidth - kTileWidth * 2;
    static constexpr uint16_t kCol9   = kScreenWidth - kTileWidth;

    // Vertical row baselines (each row is 30 px tall)
    static constexpr uint16_t kRow1 = 0;
    static constexpr uint16_t kRow2 = 30;
    static constexpr uint16_t kRow3 = 60;
    static constexpr uint16_t kRow4 = 90;
    static constexpr uint16_t kRow5 = 120;
    static constexpr uint16_t kRow6 = 150;

    // Row height shorthand
    static constexpr uint16_t kRowH = 30;  // kRow(n+1) - kRow(n)

    // Maximum number of disk-drive tiles that can be displayed simultaneously
    static constexpr size_t kMaxDiskWidgets = 10;

    // Fan indices into PcMetrics::system_fans[]
    static constexpr uint8_t kSystemFan1Index = 0;
    static constexpr uint8_t kSystemFan2Index = 1;

    // -----------------------------------------------------------------------
    // Dependencies
    // -----------------------------------------------------------------------
    DisplayContext& context_;
    PcMetrics& pcMetrics_;
    AppConfigInterface& config_;
    ApplicationMetrics& systemMetrics_;

    // -----------------------------------------------------------------------
    // State
    // -----------------------------------------------------------------------
    unsigned long lastUpdateTimestamp_ = 0;
    unsigned long staleTimeoutMs_      = 5000;
    bool wasFreshData_     = false;
    bool dataWasAvailable_ = false;
    bool isStaticDrawn_    = false;

    // -----------------------------------------------------------------------
    // Child widgets — grouped by subsystem for readability
    // -----------------------------------------------------------------------

    // CPU row
    std::unique_ptr<MetricWidget> cpuLoadWidget_;
    std::unique_ptr<MetricWidget> cpuTemperatureWidget_;
    std::unique_ptr<MetricWidget> cpuPowerWidget_;
    std::unique_ptr<MetricWidget> cpuFanWidget_;

    // GPU rows
    std::unique_ptr<MetricWidget> gpuLoadWidget_;
    std::unique_ptr<MetricWidget> gpuTemperatureWidget_;
    std::unique_ptr<MetricWidget> gpuPowerWidget_;
    std::unique_ptr<MetricWidget> gpu3dWidget_;
    std::unique_ptr<MetricWidget> gpuComputeWidget_;
    std::unique_ptr<MetricWidget> gpuMemoryWidget_;
    std::unique_ptr<MetricWidget> gpuFanWidget_;

    // RAM
    std::unique_ptr<MetricWidget> memoryLoadWidget_;

    // System fans
    std::unique_ptr<MetricWidget> fanWidget1_;
    std::unique_ptr<MetricWidget> fanWidget2_;

    // Disk drives (created dynamically when data first arrives)
    std::vector<std::unique_ptr<MetricWidget>> diskDriveWidgets_;

    // -----------------------------------------------------------------------
    // Construction helpers — each builds one logical group of widgets
    // -----------------------------------------------------------------------
    void buildCpuWidgets();
    void buildGpuWidgets();
    void buildMemoryWidget();
    void buildFanWidgets();

    // -----------------------------------------------------------------------
    // Runtime helpers
    // -----------------------------------------------------------------------
    void drawDynamicData();
    void drawNoDataMessage();
    void clearAllWidgets();
    void restoreStaticDisplay();

    bool hasFreshData() const;
    void markDataFresh() { wasFreshData_ = true; }
    void markDataStale() { wasFreshData_ = false; dataWasAvailable_ = false; }
    void showStaleIndicator();

    void createDiskDriveWidgets();
    void ensureDiskWidgetsCreated();
    void updateDiskDriveWidgets();
};
