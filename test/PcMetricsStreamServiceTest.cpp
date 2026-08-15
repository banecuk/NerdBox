#include <cstring>

#include "config/AppSettings.h"
#include "NullLogger.h"
#include "PcMetricsStreamService.h"

#include <gtest/gtest.h>

namespace {

SseEventParser::Event makeEvent(const char* json) {
    SseEventParser::Event event;
    event.data = json;
    event.dataLen = strlen(json);
    return event;
}

}  // namespace

// Delta-mode publish rule: "any section present is enough," unlike the
// polling path's "no present section failed" (PcMetricsService::parseData).

TEST(PcMetricsStreamServiceTest, PublishesWhenAnySectionPresent) {
    PcMetrics metrics;
    AppSettings config;
    NullLogger logger;
    PcMetricsStreamService service(metrics, config, logger);

    EXPECT_FALSE(metrics.freshness.available());

    SseEventParser::Event event = makeEvent(R"({"Metrics": {"Cpu": {"Load": 42.5}}})");
    service.handleEvent(event);

    EXPECT_TRUE(metrics.freshness.available());
    EXPECT_EQ(metrics.cpu_load, 42);
}

TEST(PcMetricsStreamServiceTest, DoesNotPublishWhenMetricsObjectMissing) {
    PcMetrics metrics;
    AppSettings config;
    NullLogger logger;
    PcMetricsStreamService service(metrics, config, logger);

    SseEventParser::Event event = makeEvent(R"({"SomethingElse": 1})");
    service.handleEvent(event);

    EXPECT_FALSE(metrics.freshness.available());
}

TEST(PcMetricsStreamServiceTest, DoesNotPublishOnMalformedJson) {
    PcMetrics metrics;
    AppSettings config;
    NullLogger logger;
    PcMetricsStreamService service(metrics, config, logger);

    SseEventParser::Event event = makeEvent("{not json");
    service.handleEvent(event);

    EXPECT_FALSE(metrics.freshness.available());
}

TEST(PcMetricsStreamServiceTest, PublishesOnPartialDeltaEvenIfOtherSectionsAbsent) {
    // A delta event carrying only one section — e.g. just Gpu — must still
    // publish, unlike the full-report polling path which requires every
    // present section to parse successfully AND (implicitly) expects a
    // complete report.
    PcMetrics metrics;
    AppSettings config;
    NullLogger logger;
    PcMetricsStreamService service(metrics, config, logger);

    SseEventParser::Event event = makeEvent(R"({"Metrics": {"Gpu": {"Load": 12.0}}})");
    service.handleEvent(event);

    EXPECT_TRUE(metrics.freshness.available());
    EXPECT_EQ(metrics.gpu_load, 12);
}
