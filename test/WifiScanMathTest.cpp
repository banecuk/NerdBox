#include "services/wifiScan/WifiScanMath.h"

#include <gtest/gtest.h>

namespace {
WifiApEntry makeEntry(int8_t rssi, uint8_t bssidId, uint8_t channel = 1,
                      bool isCurrent = false) {
    WifiApEntry e;
    e.rssi = rssi;
    e.channel = channel;
    e.isCurrent = isCurrent;
    for (auto& b : e.bssid) {
        b = 0;
    }
    e.bssid[5] = bssidId;  // distinct entries just need distinct last bytes
    return e;
}
}  // namespace

// ─── insertSorted ───────────────────────────────────────────────────────────

TEST(WifiScanMathTest, InsertsStrongestFirst) {
    WifiApEntry entries[WifiScanData::kMaxEntries];
    uint8_t count = 0;

    WifiScanMath::insertSorted(entries, count, makeEntry(-70, 1));
    WifiScanMath::insertSorted(entries, count, makeEntry(-40, 2));
    WifiScanMath::insertSorted(entries, count, makeEntry(-90, 3));

    ASSERT_EQ(count, 3);
    EXPECT_EQ(entries[0].rssi, -40);
    EXPECT_EQ(entries[1].rssi, -70);
    EXPECT_EQ(entries[2].rssi, -90);
}

TEST(WifiScanMathTest, TopNCutKeepsStrongestNotFirstSeen) {
    WifiApEntry entries[WifiScanData::kMaxEntries];
    uint8_t count = 0;

    // Fill to capacity with mediocre signals, all seen before the strong one.
    for (uint8_t i = 0; i < WifiScanData::kMaxEntries; ++i) {
        WifiScanMath::insertSorted(entries, count, makeEntry(-70, i));
    }
    ASSERT_EQ(count, WifiScanData::kMaxEntries);

    // A much stronger AP arrives last — it must still make the cut, bumping
    // the weakest entry rather than being dropped for arriving late.
    WifiScanMath::insertSorted(entries, count,
                               makeEntry(-20, WifiScanData::kMaxEntries));
    EXPECT_EQ(count, WifiScanData::kMaxEntries);
    EXPECT_EQ(entries[0].rssi, -20);

    // A weaker-than-everything-kept AP must not displace anything.
    WifiScanMath::insertSorted(entries, count,
                               makeEntry(-95, WifiScanData::kMaxEntries + 1));
    EXPECT_EQ(count, WifiScanData::kMaxEntries);
    for (uint8_t i = 0; i < count; ++i) {
        EXPECT_NE(entries[i].rssi, -95);
    }
}

TEST(WifiScanMathTest, DuplicateBssidKeepsStrongerReading) {
    WifiApEntry entries[WifiScanData::kMaxEntries];
    uint8_t count = 0;

    WifiScanMath::insertSorted(entries, count, makeEntry(-80, 1));
    WifiScanMath::insertSorted(entries, count, makeEntry(-50, 1));  // same BSSID, stronger

    ASSERT_EQ(count, 1);
    EXPECT_EQ(entries[0].rssi, -50);

    // A weaker repeat of the same BSSID must not overwrite the stronger one.
    WifiScanMath::insertSorted(entries, count, makeEntry(-90, 1));
    ASSERT_EQ(count, 1);
    EXPECT_EQ(entries[0].rssi, -50);
}

TEST(WifiScanMathTest, DistinctBssidsSameSsidAreKeptSeparately) {
    // Mesh/repeater case: same conceptual network, different radios.
    WifiApEntry entries[WifiScanData::kMaxEntries];
    uint8_t count = 0;

    WifiScanMath::insertSorted(entries, count, makeEntry(-60, 1));
    WifiScanMath::insertSorted(entries, count, makeEntry(-65, 2));

    EXPECT_EQ(count, 2);
}

// ─── rssiToQuality ───────────────────────────────────────────────────────────

TEST(WifiScanMathTest, QualityClampsAtStrongEnd) {
    EXPECT_EQ(WifiScanMath::rssiToQuality(-20), 100);
    EXPECT_EQ(WifiScanMath::rssiToQuality(-30), 100);
}

TEST(WifiScanMathTest, QualityClampsAtWeakEnd) {
    EXPECT_EQ(WifiScanMath::rssiToQuality(-90), 0);
    EXPECT_EQ(WifiScanMath::rssiToQuality(-110), 0);
}

TEST(WifiScanMathTest, QualityIsLinearBetweenBounds) {
    EXPECT_EQ(WifiScanMath::rssiToQuality(-60), 50);
}

// ─── signalTier ──────────────────────────────────────────────────────────────

TEST(WifiScanMathTest, SignalTierBoundaries) {
    EXPECT_EQ(WifiScanMath::signalTier(-50), WifiScanMath::SignalTier::kStrong);
    EXPECT_EQ(WifiScanMath::signalTier(-65), WifiScanMath::SignalTier::kWarn);  // boundary itself
    EXPECT_EQ(WifiScanMath::signalTier(-70), WifiScanMath::SignalTier::kWarn);
    EXPECT_EQ(WifiScanMath::signalTier(-75), WifiScanMath::SignalTier::kDegraded);
    EXPECT_EQ(WifiScanMath::signalTier(-80), WifiScanMath::SignalTier::kDegraded);
    EXPECT_EQ(WifiScanMath::signalTier(-85), WifiScanMath::SignalTier::kWeak);
    EXPECT_EQ(WifiScanMath::signalTier(-95), WifiScanMath::SignalTier::kWeak);
}

// ─── countOnChannel ──────────────────────────────────────────────────────────

TEST(WifiScanMathTest, CountsNeighboursOnSameChannelExcludingSelf) {
    WifiApEntry entries[4] = {
        makeEntry(-50, 1, 6, /*isCurrent=*/true),   // us — must not count
        makeEntry(-60, 2, 6, /*isCurrent=*/false),  // same channel
        makeEntry(-70, 3, 6, /*isCurrent=*/false),  // same channel
        makeEntry(-80, 4, 11, /*isCurrent=*/false),  // different channel
    };

    EXPECT_EQ(WifiScanMath::countOnChannel(entries, 4, 6), 2);
    EXPECT_EQ(WifiScanMath::countOnChannel(entries, 4, 11), 1);
    EXPECT_EQ(WifiScanMath::countOnChannel(entries, 4, 1), 0);
}

// ─── authName ────────────────────────────────────────────────────────────────

TEST(WifiScanMathTest, AuthNameCoversEveryKnownMode) {
    EXPECT_STREQ(WifiScanMath::authName(0), "OPEN");
    EXPECT_STREQ(WifiScanMath::authName(1), "WEP");
    EXPECT_STREQ(WifiScanMath::authName(2), "WPA");
    EXPECT_STREQ(WifiScanMath::authName(3), "WPA2");
    EXPECT_STREQ(WifiScanMath::authName(4), "WPA2");
    EXPECT_STREQ(WifiScanMath::authName(6), "WPA3");
    EXPECT_STREQ(WifiScanMath::authName(7), "WPA2/3");
}

TEST(WifiScanMathTest, AuthNameFallsBackForUnmodeledModes) {
    EXPECT_STREQ(WifiScanMath::authName(5), "?");  // enterprise
    EXPECT_STREQ(WifiScanMath::authName(8), "?");  // WAPI
    EXPECT_STREQ(WifiScanMath::authName(9), "?");  // OWE
    EXPECT_STREQ(WifiScanMath::authName(200), "?");
}
