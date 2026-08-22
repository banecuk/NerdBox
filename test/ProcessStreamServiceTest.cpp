#include <cstring>
#include <string>

#include "NullLogger.h"
#include "ProcessData.h"
#include "ProcessStreamService.h"

#include <gtest/gtest.h>

namespace {

SseEventParser::Event makeEvent(const char* json) {
    SseEventParser::Event event;
    event.data = json;
    event.dataLen = strlen(json);
    return event;
}

}  // namespace

TEST(ProcessStreamServiceTest, CpuPercentNullBecomesMinusOne) {
    ProcessData data;
    NullLogger logger;
    ProcessStreamService service(data, logger);

    SseEventParser::Event event = makeEvent(
        R"({"TopByCpu":[{"Name":"idle","Pid":1,"CpuPercent":null,"RamMB":10,"DiskKBPerSec":0}]})");
    service.handleEvent(event);

    ASSERT_EQ(data.byCpuCount, 1);
    EXPECT_FLOAT_EQ(data.byCpu[0].cpuPercent, -1.0f);
}

TEST(ProcessStreamServiceTest, FewerThanEightEntriesGivesCorrectCount) {
    ProcessData data;
    NullLogger logger;
    ProcessStreamService service(data, logger);

    SseEventParser::Event event = makeEvent(
        R"({"TopByCpu":[{"Name":"a","Pid":1,"CpuPercent":1.0,"RamMB":1,"DiskKBPerSec":0},
                          {"Name":"b","Pid":2,"CpuPercent":2.0,"RamMB":2,"DiskKBPerSec":0}]})");
    service.handleEvent(event);

    EXPECT_EQ(data.byCpuCount, 2);
    EXPECT_STREQ(data.byCpu[0].name, "a");
    EXPECT_STREQ(data.byCpu[1].name, "b");
}

TEST(ProcessStreamServiceTest, MoreThanEightEntriesIsCapped) {
    ProcessData data;
    NullLogger logger;
    ProcessStreamService service(data, logger);

    std::string json = R"({"TopByCpu":[)";
    for (int i = 0; i < 12; ++i) {
        if (i > 0)
            json += ",";
        json += R"({"Name":"p)" + std::to_string(i) +
                R"(","Pid":)" + std::to_string(i) +
                R"(,"CpuPercent":1.0,"RamMB":1,"DiskKBPerSec":0})";
    }
    json += "]}";

    SseEventParser::Event event = makeEvent(json.c_str());
    service.handleEvent(event);

    EXPECT_EQ(data.byCpuCount, ProcessData::kEntriesPerList);
}

TEST(ProcessStreamServiceTest, LongNameIsTruncatedAndNulTerminated) {
    ProcessData data;
    NullLogger logger;
    ProcessStreamService service(data, logger);

    SseEventParser::Event event = makeEvent(
        R"({"TopByCpu":[{"Name":"a_very_long_process_name_that_exceeds_the_buffer","Pid":1,)"
        R"("CpuPercent":1.0,"RamMB":1,"DiskKBPerSec":0}]})");
    service.handleEvent(event);

    ASSERT_EQ(data.byCpuCount, 1);
    EXPECT_EQ(strlen(data.byCpu[0].name), sizeof(data.byCpu[0].name) - 1);
}

TEST(ProcessStreamServiceTest, AbsentListLeavesPreviousValuesUntouched) {
    ProcessData data;
    NullLogger logger;
    ProcessStreamService service(data, logger);

    SseEventParser::Event first = makeEvent(
        R"({"TopByCpu":[{"Name":"a","Pid":1,"CpuPercent":5.0,"RamMB":1,"DiskKBPerSec":0}]})");
    service.handleEvent(first);
    ASSERT_EQ(data.byCpuCount, 1);

    SseEventParser::Event second = makeEvent(
        R"({"TopByRam":[{"Name":"b","Pid":2,"CpuPercent":3.0,"RamMB":2,"DiskKBPerSec":0}]})");
    service.handleEvent(second);

    EXPECT_EQ(data.byCpuCount, 1);
    EXPECT_STREQ(data.byCpu[0].name, "a");
    EXPECT_EQ(data.byRamCount, 1);
    EXPECT_STREQ(data.byRam[0].name, "b");
}

TEST(ProcessStreamServiceTest, MalformedJsonDoesNotPublishOrCrash) {
    ProcessData data;
    NullLogger logger;
    ProcessStreamService service(data, logger);

    SseEventParser::Event event = makeEvent("{not json");
    service.handleEvent(event);

    EXPECT_FALSE(data.freshness.available());
    EXPECT_EQ(data.byCpuCount, 0);
}
