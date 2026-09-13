#pragma once

#include "config/AppSettings.h"
#include "network/NetworkManager.h"
#include "services/wifiScan/WifiScanData.h"
#include "ui/core/DisplayContext.h"
#include "ui/widgets/base/Widget.h"
#include "utils/RingHistory.h"

// This device's own WiFi link — SSID/security/channel/co-channel-neighbour
// count, signal bars + dBm + quality, a 60 s rolling RSSI sparkline, and the
// IP/gateway/mask/DNS/MAC/hostname details strip. Content of WifiScreen's top
// 0..119 band. See docs-local/13-wifi-screen-plan.md §4.2.
//
// Reads NetworkManager::fillLinkInfo() each tick (its own update interval —
// see AppConfig::WifiScanImpl::kRssiTraceIntervalMs) rather than a shared
// data struct, since this is the only widget that needs it (§2.2's rationale
// for not growing DataBundle with it). WifiScanData is read only for
// sameChannelCount.
class WifiLinkWidget : public Widget {
 public:
    WifiLinkWidget(DisplayContext& context, const WidgetInterface::Dimensions& dims,
                   uint32_t updateIntervalMs, NetworkManager& networkManager,
                   const WifiScanData& wifiScan, const AppSettings& config);

 protected:
    void onDraw(bool forceRedraw) override;
    void onDrawStatic() override;

 private:
    static constexpr uint16_t kCardH = 76;    // SSID/bars/trace row
    static constexpr uint16_t kDetailsH = 42;  // IP/GW/MASK/DNS/MAC/HOST strip
    static constexpr uint16_t kHairlineY = kCardH + kDetailsH;  // 118, relative to dims_.y

    static constexpr int8_t kTraceMinDbm = -90;
    static constexpr int8_t kTraceMaxDbm = -30;
    static constexpr size_t kTraceLen = 60;
    static constexpr uint16_t kTraceColW = 2;

    NetworkManager& networkManager_;
    const WifiScanData& wifiScan_;
    const AppSettings& config_;

    RingHistory<int8_t, kTraceLen> rssiTrace_;
    bool lastConnected_ = false;

    // Cached details-strip text, redrawn only on change (these barely ever
    // change once connected).
    char lastIp_[16] = "";
    char lastGateway_[16] = "";
    char lastMask_[16] = "";
    char lastDns_[16] = "";
    char lastMac_[18] = "";
    char lastHostname_[64] = "";
    char lastSsid_[33] = "";

    // Cached values gating the subtitle line and bars/dBm/trace/quality —
    // without these, drawTopRow()/drawTrace() cleared-then-redrew identical
    // text/pixels every tick (1s), which visibly flashed on real hardware
    // even though nothing had changed.
    uint8_t lastAuth_ = 0xFF;
    uint8_t lastChannel_ = 0xFF;
    uint8_t lastNeighbourCount_ = 0xFF;
    bool lastNeighbourAvailable_ = false;  // has a scan ever completed?
    int8_t lastRssi_ = 127;  // outside the valid dBm range — forces first draw

    void drawDisconnected();
    void drawTopRow(const NetworkManager::LinkInfo& link, bool forceRedraw);
    void drawDetailsStrip(const NetworkManager::LinkInfo& link, bool forceRedraw);
    void drawTrace();
};
