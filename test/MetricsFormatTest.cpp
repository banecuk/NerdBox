#include <ArduinoJson.h>

#include <cstring>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "config/AppSettings.h"
#include "services/web/WebEmitters.h"

#include <gtest/gtest.h>
#include <Print.h>

// Host tests for the pure /metrics, /api/status and /api/pc emitters (A3).
// Covers the bug classes B45 (duplicate HELP/TYPE), B65 (phantom thread
// entries) and B66 (unescaped labels, samples for feeds with no data).

namespace {

class FakeStream : public IStreamHealth {
 public:
    explicit FakeStream(const char* state) : state_(state) {}
    const char* stateName() const override { return state_; }
    uint32_t reconnectCount() const override { return 3; }
    uint32_t lastEventAgeMs() const override { return 120; }
    uint32_t overflowCount() const override { return 1; }

 private:
    const char* state_;
};

class FakeStacks : public ITaskStackReporter {
 public:
    uint32_t screenTaskStackFree() const override { return 1111; }
    uint32_t backgroundTaskStackFree() const override { return 2222; }
    bool watchdogActive() const override { return true; }
};

struct Fixture {
    AppSettings config;
    ApplicationMetrics systemMetrics;
    PcMetrics pcMetrics;
    FakeStream pcStream{"CONNECTED"};
    FakeStream cpuClockStream{"DISCONNECTED"};
    FakeStream processStream{"DISCONNECTED"};
    NetworkStatus netStatus;
    SystemState systemState;
    WeatherData weatherData;
    RoomClimateData roomClimateData;
    AudioData audioData;
    WifiScanData wifiScanData;
    FakeStacks stacks;

    WebFeeds feeds{config,        systemMetrics, pcMetrics,   pcStream,    cpuClockStream,
                   processStream, netStatus,     systemState, weatherData, roomClimateData,
                   audioData,     wifiScanData,  stacks};
    StatusSnapshot snap;

    // Every optional series populated, so the "one HELP/TYPE per name" check
    // sees the full metric set.
    void populateAll(bool roomReached = true) {
        snap.nowMs = 10000;
        snap.uptimeMs = 123456;
        snap.heapFree = 200000;
        snap.psramFree = 4000000;
        snap.pcFresh = true;
        snap.link.connected = true;
        snap.link.rssi = -55;
        snap.link.channel = 6;

        pcMetrics.cpu_core_count = 28;
        for (int i = 0; i < AppConfig::Limits::kMaxThreads; ++i) {
            pcMetrics.cpu_thread_load[i] = static_cast<uint8_t>(i + 1);
        }
        pcMetrics.system_fan_count = 2;
        pcMetrics.system_fans[0] = 900;
        pcMetrics.system_fans[1] = 1200;
        pcMetrics.disk_drives.push_back(makeDrive("C", 40.0f));
        pcMetrics.disk_drives.push_back(makeDrive("D", 70.0f));
        pcMetrics.freshness.publish(9000);

        roomClimateData.temperature_x10 = 215;
        roomClimateData.humidity = 45;
        if (roomReached) {
            roomClimateData.freshness.publish(9500);
        }
        weatherData.freshness.publish(5000);
    }

    static DiskDrive makeDrive(const char* name, float freePct) {
        DiskDrive d{};
        strncpy(d.driveName, name, sizeof(d.driveName) - 1);
        d.freeSpacePercent = freePct;
        d.readKBPerSec = 1.5f;
        d.writeKBPerSec = 2.5f;
        return d;
    }

    std::string metrics() {
        StringPrint out;
        WebEmitters::writeMetrics(out, snap, feeds);
        return out.str();
    }
};

std::vector<std::string> lines(const std::string& text) {
    std::vector<std::string> out;
    std::istringstream in(text);
    std::string line;
    while (std::getline(in, line)) {
        out.push_back(line);
    }
    return out;
}

// Metric name of a sample line: everything up to '{' or ' '.
std::string sampleName(const std::string& line) {
    const size_t end = line.find_first_of("{ ");
    return line.substr(0, end);
}

size_t countPrefix(const std::vector<std::string>& all, const std::string& prefix) {
    size_t n = 0;
    for (const std::string& l : all) {
        if (l.compare(0, prefix.size(), prefix) == 0) {
            ++n;
        }
    }
    return n;
}

}  // namespace

TEST(MetricsFormatTest, EveryMetricHasExactlyOneHelpAndOneType) {
    Fixture f;
    f.populateAll();
    const std::vector<std::string> all = lines(f.metrics());
    ASSERT_FALSE(all.empty());

    std::map<std::string, int> help;
    std::map<std::string, int> type;
    std::set<std::string> sampled;
    for (size_t i = 0; i < all.size(); ++i) {
        const std::string& l = all[i];
        ASSERT_FALSE(l.empty()) << "blank line at " << i;
        if (l.rfind("# HELP ", 0) == 0) {
            const std::string name = l.substr(7, l.find(' ', 7) - 7);
            ++help[name];
            // TYPE must immediately follow its HELP.
            ASSERT_LT(i + 1, all.size());
            EXPECT_EQ(all[i + 1].rfind("# TYPE " + name + " ", 0), 0u) << name;
        } else if (l.rfind("# TYPE ", 0) == 0) {
            const std::string name = l.substr(7, l.find(' ', 7) - 7);
            ++type[name];
            const std::string kind = l.substr(l.rfind(' ') + 1);
            EXPECT_TRUE(kind == "gauge" || kind == "counter") << l;
        } else {
            ASSERT_NE(l[0], '#') << l;
            sampled.insert(sampleName(l));
        }
    }

    for (const auto& entry : help) {
        EXPECT_EQ(entry.second, 1) << "duplicate HELP for " << entry.first;
    }
    for (const auto& entry : type) {
        EXPECT_EQ(entry.second, 1) << "duplicate TYPE for " << entry.first;
    }
    EXPECT_EQ(help.size(), type.size());
    for (const std::string& name : sampled) {
        EXPECT_EQ(help.count(name), 1u) << "sample without HELP: " << name;
        EXPECT_EQ(type.count(name), 1u) << "sample without TYPE: " << name;
    }
    // Every declared metric actually has a sample when every feed has data.
    EXPECT_EQ(sampled.size(), help.size());
}

TEST(MetricsFormatTest, ByteCompatibleSamples) {
    Fixture f;
    f.populateAll();
    const std::string text = f.metrics();
    EXPECT_NE(text.find("# HELP nerdbox_uptime_seconds Device uptime.\n"
                        "# TYPE nerdbox_uptime_seconds counter\n"
                        "nerdbox_uptime_seconds 123.456\n"),
              std::string::npos);
    EXPECT_NE(text.find("nerdbox_heap_free_bytes 200000\n"), std::string::npos);
    EXPECT_NE(text.find("nerdbox_stream_reconnects_total{stream=\"cpu_clock\"} 3\n"),
              std::string::npos);
    EXPECT_NE(text.find("nerdbox_pc_gpu_fps -1\n"), std::string::npos);
    EXPECT_NE(text.find("nerdbox_pc_mem_swap_used_gigabytes 0.00\n"), std::string::npos);
    EXPECT_NE(text.find("nerdbox_pc_system_fan_rpm{fan=\"1\"} 1200\n"), std::string::npos);
    EXPECT_NE(text.find("nerdbox_pc_disk_free_percent{drive=\"D\"} 70.0\n"), std::string::npos);
    EXPECT_NE(text.find("nerdbox_room_temperature_celsius 21.5\n"), std::string::npos);
    EXPECT_NE(text.find("nerdbox_wifi_channel 6\n"), std::string::npos);
}

TEST(MetricsFormatTest, CpuThreadLoadHasCoreCountEntries) {
    Fixture f;
    f.populateAll();
    const std::vector<std::string> all = lines(f.metrics());
    EXPECT_EQ(countPrefix(all, "nerdbox_pc_cpu_thread_load_percent{"), 28u);
    EXPECT_EQ(countPrefix(all, "nerdbox_pc_cpu_thread_load_percent{thread=\"27\"} 28"), 1u);
    EXPECT_EQ(countPrefix(all, "nerdbox_pc_cpu_thread_load_percent{thread=\"28\"}"), 0u);

    JsonDocument doc;
    WebEmitters::writePc(doc, f.feeds);
    EXPECT_EQ(doc["cpu_thread_load"].as<JsonArrayConst>().size(), 28u);
}

TEST(MetricsFormatTest, CpuThreadLoadClampedToBackingArray) {
    Fixture f;
    f.populateAll();
    f.pcMetrics.cpu_core_count = 200;  // more than kMaxThreads
    const std::vector<std::string> all = lines(f.metrics());
    EXPECT_EQ(countPrefix(all, "nerdbox_pc_cpu_thread_load_percent{"),
              static_cast<size_t>(AppConfig::Limits::kMaxThreads));
}

TEST(MetricsFormatTest, NoThreadSamplesBeforeFirstCoreLoads) {
    Fixture f;
    const std::vector<std::string> all = lines(f.metrics());
    EXPECT_EQ(countPrefix(all, "nerdbox_pc_cpu_thread_load_percent{"), 0u);
    EXPECT_EQ(countPrefix(all, "# HELP nerdbox_pc_cpu_thread_load_percent "), 1u);
}

TEST(MetricsFormatTest, DriveLabelsAreEscaped) {
    Fixture f;
    f.populateAll();
    f.pcMetrics.disk_drives.clear();
    f.pcMetrics.disk_drives.push_back(Fixture::makeDrive("a\"\\", 10.0f));
    const std::string text = f.metrics();
    EXPECT_NE(text.find("nerdbox_pc_disk_free_percent{drive=\"a\\\"\\\\\"} 10.0\n"),
              std::string::npos)
        << text;
    EXPECT_EQ(text.find("drive=\"a\"\\\""), std::string::npos);
}

TEST(MetricsFormatTest, EscapeLabelValue) {
    char out[16];
    EXPECT_EQ(WebEmitters::escapeLabelValue("plain", out, sizeof(out)), 5u);
    EXPECT_STREQ(out, "plain");

    WebEmitters::escapeLabelValue("a\\b\"c\nd", out, sizeof(out));
    EXPECT_STREQ(out, "a\\\\b\\\"c\\nd");

    // Truncation never splits an escape sequence in half.
    char small[3];
    EXPECT_EQ(WebEmitters::escapeLabelValue("a\"", small, sizeof(small)), 1u);
    EXPECT_STREQ(small, "a");

    EXPECT_EQ(WebEmitters::escapeLabelValue(nullptr, out, sizeof(out)), 0u);
    EXPECT_STREQ(out, "");
}

TEST(MetricsFormatTest, FeedsWithoutDataEmitNoSamples) {
    Fixture f;
    f.populateAll(/*roomReached=*/false);
    f.snap.link.connected = false;
    const std::vector<std::string> all = lines(f.metrics());

    // HELP/TYPE stay (the metric set is stable), samples go.
    EXPECT_EQ(countPrefix(all, "# HELP nerdbox_room_temperature_celsius "), 1u);
    EXPECT_EQ(countPrefix(all, "nerdbox_room_temperature_celsius "), 0u);
    EXPECT_EQ(countPrefix(all, "nerdbox_room_humidity_percent "), 0u);
    EXPECT_EQ(countPrefix(all, "nerdbox_wifi_quality_percent "), 0u);
    EXPECT_EQ(countPrefix(all, "nerdbox_wifi_channel "), 0u);
    // Explicit availability/connection gauges still say why.
    EXPECT_EQ(countPrefix(all, "nerdbox_wifi_connected "), 1u);
}

TEST(StatusJsonTest, WritesEveryFeedObjectFromSnapshot) {
    Fixture f;
    f.populateAll();
    f.snap.heapMin = 150000;
    f.snap.sdk = "v-test";
    f.snap.pcFetchOk = 7;
    strncpy(f.snap.pcLastError, "http: code -1", sizeof(f.snap.pcLastError) - 1);
    strncpy(f.snap.netIp, "192.168.1.50", sizeof(f.snap.netIp) - 1);
    const uint8_t bssid[6] = {0xAA, 0xBB, 0xCC, 0x01, 0x02, 0x03};
    memcpy(f.snap.link.bssid, bssid, sizeof(bssid));
    f.systemState.screen.activeScreen = ScreenName::MAIN;
    f.audioData.stopped = true;
    f.audioData.stoppedAtMs = 7000;

    JsonDocument doc;
    WebEmitters::writeStatus(doc, f.snap, f.feeds);

    // Top-level key order is part of the contract.
    const char* expectedKeys[] = {
        "sys",     "app",  "net",  "pc",    "pc_stream", "cpu_clock_stream", "process_stream", "ui",
        "weather", "room", "wifi", "audio", "tasks"};
    size_t idx = 0;
    for (JsonPairConst kv : doc.as<JsonObjectConst>()) {
        ASSERT_LT(idx, sizeof(expectedKeys) / sizeof(expectedKeys[0]));
        EXPECT_STREQ(kv.key().c_str(), expectedKeys[idx]);
        ++idx;
    }
    EXPECT_EQ(idx, sizeof(expectedKeys) / sizeof(expectedKeys[0]));

    EXPECT_EQ(doc["sys"]["heap_min"].as<uint32_t>(), 150000u);
    EXPECT_STREQ(doc["sys"]["sdk"].as<const char*>(), "v-test");
    EXPECT_EQ(doc["app"]["uptime_ms"].as<uint64_t>(), 123456u);
    EXPECT_STREQ(doc["net"]["ip"].as<const char*>(), "192.168.1.50");
    EXPECT_EQ(doc["net"]["probes"].as<JsonArrayConst>().size(),
              static_cast<size_t>(AppConfig::Limits::kNetworkProbeEndpoints));
    EXPECT_TRUE(doc["pc"]["fresh"].as<bool>());
    EXPECT_EQ(doc["pc"]["age_ms"].as<unsigned long>(), 1000u);
    EXPECT_EQ(doc["pc"]["fetch_ok"].as<uint32_t>(), 7u);
    EXPECT_STREQ(doc["pc"]["last_error"].as<const char*>(), "http: code -1");
    EXPECT_STREQ(doc["pc_stream"]["state"].as<const char*>(), "CONNECTED");
    EXPECT_EQ(doc["process_stream"]["reconnect_count"].as<uint32_t>(), 3u);
    EXPECT_STREQ(doc["ui"]["screen"].as<const char*>(), "MAIN");
    EXPECT_EQ(doc["room"]["temperature_x10"].as<int>(), 215);
    EXPECT_EQ(doc["room"]["last_event_age_ms"].as<unsigned long>(), 500u);
    EXPECT_STREQ(doc["wifi"]["bssid"].as<const char*>(), "AA:BB:CC:01:02:03");
    EXPECT_EQ(doc["wifi"]["channel"].as<int>(), 6);
    EXPECT_EQ(doc["audio"]["stopped_age_ms"].as<unsigned long>(), 3000u);
    EXPECT_EQ(doc["tasks"]["background_stack_free"].as<uint32_t>(), 2222u);
}

TEST(StatusJsonTest, PcJsonCarriesDrivesAndFans) {
    Fixture f;
    f.populateAll();
    JsonDocument doc;
    WebEmitters::writePc(doc, f.feeds);
    EXPECT_TRUE(doc["available"].as<bool>());
    EXPECT_EQ(doc["last_update_timestamp"].as<unsigned long>(), 9000u);
    EXPECT_EQ(doc["gpu_fps"].as<int>(), -1);
    EXPECT_EQ(doc["system_fans"].as<JsonArrayConst>().size(), 2u);
    ASSERT_EQ(doc["disk_drives"].as<JsonArrayConst>().size(), 2u);
    EXPECT_STREQ(doc["disk_drives"][0]["name"].as<const char*>(), "C");
    EXPECT_FLOAT_EQ(doc["disk_drives"][0]["read_kbps"].as<float>(), 1.5f);
}
