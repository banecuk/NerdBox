#include <ArduinoJson.h>

#include "NullLogger.h"
#include "PcMetricsParser.h"

#include <gtest/gtest.h>

// ─── Helpers ────────────────────────────────────────────────────────────────

// Parses `json` and returns the "Metrics" object it contains. The returned
// JsonDocument must outlive any JsonObjectConst taken from it, so callers
// keep the document alive on their own stack.
static JsonObjectConst metricsOf(JsonDocument& doc, const char* json) {
    deserializeJson(doc, json);
    return doc["Metrics"];
}

// A PcMetrics pre-loaded with sentinel values distinct from anything a test
// fixture below sets, so "left untouched" is unambiguous from "coincidentally
// matches the fixture's value".
static void seedSentinels(PcMetrics& m) {
    m.cpu_load = 11;
    m.gpu_load = 22;
    m.mem_load = 33;
    m.cpu_temperature = 44;
    m.gpu_temperature = 55;
    m.cpu_power = 111;
    m.gpu_power = 222;
    m.cpu_fan = 333;
    m.gpu_fan = 444;
    m.gpu_fps = 999;
    m.eth_up = 1.5f;
    m.eth_dn = 2.5f;
    for (auto& v : m.cpu_thread_load)
        v = 77;
}

// ─── Full report ────────────────────────────────────────────────────────────

static const char* kFullReportJson = R"({
  "Metrics": {
    "Cpu": {"Load": 42.5, "CoreLoads": [10,20,30,40,50,60,70,80,90,100,15,25,35,45,55,65,75,85,95,5,12,22,32,42]},
    "CpuExtended": {"TemperatureCoreMax": 65.3, "PackagePower": 95.7},
    "Ram": {"Load": 55.1},
    "Gpu": {"Load": 30.2, "Temperature": 60.9, "PackagePower": 120.4, "Fan": 1500.0,
            "D3d3d": 12.0, "D3dCompute": 3.0, "D3dVideoDecode": 7.0, "MemoryLoad": 50.0,
            "FullscreenFps": 155.40016},
    "Motherboard": {"CpuFan": 1200.0, "SystemFans": [800, 0, 900]},
    "Disks": {"Drives": [{"DriveName":"C","FreeSpacePercent":45.5,"ReadKBPerSec":120.0,"WriteKBPerSec":30.0}]},
    "Network": {"TotalUploadKBPerSec": 512.0, "TotalDownloadKBPerSec": 2048.0}
  }
})";

TEST(PcMetricsParserTest, FullReportParsesEverySection) {
    JsonDocument doc;
    JsonObjectConst metrics = metricsOf(doc, kFullReportJson);

    PcMetrics m;
    NullLogger logger;
    auto result = PcMetricsParser::parseAllSections(metrics, m, 24, logger);

    EXPECT_TRUE(result.allSeenValid());
    EXPECT_EQ(result.sectionsSeen, PcMetricsParser::kCpu | PcMetricsParser::kCpuExtended |
                                       PcMetricsParser::kRam | PcMetricsParser::kGpu |
                                       PcMetricsParser::kMotherboard | PcMetricsParser::kDisks |
                                       PcMetricsParser::kNetwork);

    EXPECT_EQ(m.cpu_load, 42);
    EXPECT_EQ(m.cpu_thread_load[0], 10);
    EXPECT_EQ(m.cpu_thread_load[23], 42);
    EXPECT_EQ(m.cpu_temperature, 65);
    EXPECT_EQ(m.cpu_power, 95);
    EXPECT_EQ(m.mem_load, 55);
    EXPECT_EQ(m.gpu_load, 30);
    EXPECT_EQ(m.gpu_temperature, 60);
    EXPECT_EQ(m.gpu_3d, 12);
    EXPECT_EQ(m.gpu_compute, 3);
    EXPECT_EQ(m.gpu_decode, 7);
    EXPECT_EQ(m.gpu_fps, 155);
    EXPECT_EQ(m.cpu_fan, 1200);
    // Zero-RPM fans (disconnected headers) are filtered out, not kept as slots.
    ASSERT_EQ(m.system_fan_count, 2);
    EXPECT_EQ(m.system_fans[0], 800);
    EXPECT_EQ(m.system_fans[1], 900);
    ASSERT_EQ(m.disk_drives.size(), 1u);
    EXPECT_STREQ(m.disk_drives[0].driveName, "C");
    EXPECT_FLOAT_EQ(m.disk_drives[0].readKBPerSec, 120.0f);
    EXPECT_FLOAT_EQ(m.eth_up, 512.0f);
    EXPECT_FLOAT_EQ(m.eth_dn, 2048.0f);
}

// ─── Delta mode: one section present ───────────────────────────────────────

TEST(PcMetricsParserTest, DeltaWithOneSectionOnlyTouchesThatSection) {
    JsonDocument doc;
    JsonObjectConst metrics = metricsOf(doc, R"({"Metrics": {"Gpu": {"Load": 77.0}}})");

    PcMetrics m;
    seedSentinels(m);
    NullLogger logger;
    auto result = PcMetricsParser::parseAllSections(metrics, m, 24, logger);

    EXPECT_EQ(result.sectionsSeen, PcMetricsParser::kGpu);
    EXPECT_TRUE(result.allSeenValid());

    EXPECT_EQ(m.gpu_load, 77);  // updated

    // Everything outside Gpu is untouched — including gpu_fps, since Gpu's
    // own section handler leaves fields whose keys are absent from the Gpu
    // object alone too (delta-mode contract applies per-field, not just
    // per-section).
    EXPECT_EQ(m.cpu_load, 11);
    EXPECT_EQ(m.mem_load, 33);
    EXPECT_EQ(m.cpu_temperature, 44);
    EXPECT_EQ(m.gpu_fps, 999);
    EXPECT_EQ(m.cpu_fan, 333);
    EXPECT_FLOAT_EQ(m.eth_up, 1.5f);
    for (auto v : m.cpu_thread_load)
        EXPECT_EQ(v, 77);
}

TEST(PcMetricsParserTest, AbsentSectionKeyLeavesFieldsUntouched) {
    JsonDocument doc;
    JsonObjectConst metrics = metricsOf(doc, R"({"Metrics": {}})");

    PcMetrics m;
    seedSentinels(m);
    NullLogger logger;
    auto result = PcMetricsParser::parseAllSections(metrics, m, 24, logger);

    EXPECT_FALSE(result.anySeen());
    EXPECT_TRUE(result.allSeenValid());
    EXPECT_EQ(m.cpu_load, 11);
    EXPECT_EQ(m.gpu_load, 22);
}

// ─── Absent-key-means-unchanged, within a present section ──────────────────

TEST(PcMetricsParserTest, AbsentFieldWithinPresentSectionMeansUnchanged) {
    JsonDocument doc;
    // Cpu section present, but only Load — CoreLoads omitted.
    JsonObjectConst metrics = metricsOf(doc, R"({"Metrics": {"Cpu": {"Load": 88.0}}})");

    PcMetrics m;
    seedSentinels(m);
    NullLogger logger;
    PcMetricsParser::parseAllSections(metrics, m, 24, logger);

    EXPECT_EQ(m.cpu_load, 88);  // present key updates
    for (auto v : m.cpu_thread_load)
        EXPECT_EQ(v, 77);  // absent key: untouched
}

TEST(PcMetricsParserTest, AbsentCoreLoadsWithLoadPresentUpdatesLoadOnly) {
    // Mirror case: CoreLoads present, Load absent.
    JsonDocument doc;
    JsonObjectConst metrics = metricsOf(doc, R"({"Metrics": {"Cpu": {"CoreLoads": [50, 60]}}})");

    PcMetrics m;
    seedSentinels(m);
    NullLogger logger;
    PcMetricsParser::parseAllSections(metrics, m, 2, logger);

    EXPECT_EQ(m.cpu_load, 11);  // absent key: untouched
    EXPECT_EQ(m.cpu_thread_load[0], 50);
    EXPECT_EQ(m.cpu_thread_load[1], 60);
}

// ─── D3dVideoDecode (GPU video-decode engine) ───────────────────────────────

TEST(PcMetricsParserTest, AbsentD3dVideoDecodeLeavesGpuDecodeUnchanged) {
    JsonDocument doc;
    JsonObjectConst metrics = metricsOf(doc, R"({"Metrics": {"Gpu": {"Load": 5.0}}})");

    PcMetrics m;
    m.gpu_decode = 42;
    NullLogger logger;
    PcMetricsParser::parseAllSections(metrics, m, 24, logger);

    EXPECT_EQ(m.gpu_decode, 42);
}

TEST(PcMetricsParserTest, PresentD3dVideoDecodeUpdatesGpuDecode) {
    JsonDocument doc;
    JsonObjectConst metrics = metricsOf(doc, R"({"Metrics": {"Gpu": {"D3dVideoDecode": 63.0}}})");

    PcMetrics m;
    NullLogger logger;
    PcMetricsParser::parseAllSections(metrics, m, 24, logger);

    EXPECT_EQ(m.gpu_decode, 63);
}

// ─── FullscreenFps float extraction ─────────────────────────────────────────

TEST(PcMetricsParserTest, FullscreenFpsExtractsFloatNotIntegerFallback) {
    JsonDocument doc;
    JsonObjectConst metrics =
        metricsOf(doc, R"({"Metrics": {"Gpu": {"FullscreenFps": 155.40016}}})");

    PcMetrics m;
    NullLogger logger;
    PcMetricsParser::parseAllSections(metrics, m, 24, logger);

    // Truncates towards zero, same as static_cast<int16_t>(155.40016f).
    EXPECT_EQ(m.gpu_fps, 155);
}

TEST(PcMetricsParserTest, AbsentFullscreenFpsLeavesPreviousValueUnchanged) {
    JsonDocument doc;
    JsonObjectConst metrics = metricsOf(doc, R"({"Metrics": {"Gpu": {"Load": 5.0}}})");

    PcMetrics m;
    m.gpu_fps = 42;
    NullLogger logger;
    PcMetricsParser::parseAllSections(metrics, m, 24, logger);

    EXPECT_EQ(m.gpu_fps, 42);
}

// ─── kCores mismatch ─────────────────────────────────────────────────────────

TEST(PcMetricsParserTest, FewerCoreLoadsThanConfiguredWarnsAndZeroPadsRest) {
    JsonDocument doc;
    JsonObjectConst metrics =
        metricsOf(doc, R"({"Metrics": {"Cpu": {"CoreLoads": [10, 20, 30, 40]}}})");

    PcMetrics m;
    seedSentinels(m);
    NullLogger logger;
    PcMetricsParser::parseAllSections(metrics, m, /*configuredCoreCount=*/8, logger);

    EXPECT_EQ(m.cpu_thread_load[0], 10);
    EXPECT_EQ(m.cpu_thread_load[3], 40);
    // Slots at/after actualCores are zeroed, not left at their stale sentinel
    // — a shrinking core count must not leave old readings behind.
    EXPECT_EQ(m.cpu_thread_load[4], 0);
    EXPECT_EQ(m.cpu_thread_load[23], 0);

    EXPECT_EQ(logger.lastWarning(), "Expected 8 cores but found 4");
}

TEST(PcMetricsParserTest, MatchingCoreCountDoesNotWarn) {
    JsonDocument doc;
    JsonObjectConst metrics = metricsOf(doc, R"({"Metrics": {"Cpu": {"CoreLoads": [1, 2]}}})");

    PcMetrics m;
    NullLogger logger;
    PcMetricsParser::parseAllSections(metrics, m, /*configuredCoreCount=*/2, logger);

    EXPECT_TRUE(logger.lastWarning().empty());
}
