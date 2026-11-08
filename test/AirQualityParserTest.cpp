#include <ArduinoJson.h>

#include "NullLogger.h"
#include "services/airQuality/AirQualityParser.h"

#include <gtest/gtest.h>

static bool parseInto(const char* json, AirQualityData& data) {
    JsonDocument doc;
    deserializeJson(doc, json);
    NullLogger logger;
    return AirQualityParser::parse(doc.as<JsonObjectConst>(), data, logger);
}

TEST(AirQualityParserTest, ParsesIntegerPayload) {
    AirQualityData d;
    ASSERT_TRUE(parseInto(R"({"data": {"current": {
        "weather": {"tp": 18, "hu": 62, "pr": 1016, "ws": 1.94, "wd": 270, "ic": "10n"},
        "pollution": {"aqius": 57}}}})",
                          d));
    EXPECT_EQ(d.temperature, 18);
    EXPECT_EQ(d.humidity, 62);
    EXPECT_EQ(d.pressure, 1016);
    EXPECT_EQ(d.wind_speed_x10, 19);
    EXPECT_EQ(d.wind_dir, 270);
    EXPECT_STREQ(d.icon_code, "10n");
    EXPECT_EQ(d.aqi_us, 57);
}

// B5: every one of these used to read as 0 via `| 0` on a float node.
TEST(AirQualityParserTest, FractionalFieldsAreReadNotZeroed) {
    AirQualityData d;
    ASSERT_TRUE(parseInto(R"({"data": {"current": {
        "weather": {"tp": 21.6, "hu": 48.4, "pr": 1013.5, "ws": 3.25, "wd": 89.9, "ic": "01d"},
        "pollution": {"aqius": 42.0}}}})",
                          d));
    EXPECT_EQ(d.temperature, 22);
    EXPECT_EQ(d.humidity, 48);
    EXPECT_EQ(d.pressure, 1014);
    EXPECT_EQ(d.wind_speed_x10, 33);
    EXPECT_EQ(d.wind_dir, 90);
    EXPECT_EQ(d.aqi_us, 42);
}

TEST(AirQualityParserTest, NegativeTemperatureRoundsAwayFromZero) {
    AirQualityData d;
    ASSERT_TRUE(parseInto(
        R"({"data": {"current": {"weather": {"tp": -4.5}, "pollution": {"aqius": 10}}}})", d));
    EXPECT_EQ(d.temperature, -5);
}

TEST(AirQualityParserTest, Aqi300BoundaryIsExact) {
    AirQualityData d;
    ASSERT_TRUE(
        parseInto(R"({"data": {"current": {"weather": {}, "pollution": {"aqius": 300}}}})", d));
    EXPECT_EQ(d.aqi_us, 300);
}

TEST(AirQualityParserTest, Aqi500TopOfScaleIsExact) {
    AirQualityData d;
    ASSERT_TRUE(
        parseInto(R"({"data": {"current": {"weather": {}, "pollution": {"aqius": 500}}}})", d));
    EXPECT_EQ(d.aqi_us, 500);
}

TEST(AirQualityParserTest, AqiAboveScaleIsKeptNotWrapped) {
    // US AQI nominally tops out at 500, but readings past it do happen in
    // extreme events. Keep the value rather than wrapping or zeroing it.
    AirQualityData d;
    ASSERT_TRUE(
        parseInto(R"({"data": {"current": {"weather": {}, "pollution": {"aqius": 999.4}}}})", d));
    EXPECT_EQ(d.aqi_us, 999);
}

TEST(AirQualityParserTest, NegativeAqiClampsToZero) {
    AirQualityData d;
    ASSERT_TRUE(
        parseInto(R"({"data": {"current": {"weather": {}, "pollution": {"aqius": -3}}}})", d));
    EXPECT_EQ(d.aqi_us, 0);
}

TEST(AirQualityParserTest, OutOfRangeTemperatureClampsInsteadOfWrapping) {
    AirQualityData d;
    ASSERT_TRUE(
        parseInto(R"({"data": {"current": {"weather": {"tp": 300}, "pollution": {}}}})", d));
    EXPECT_EQ(d.temperature, 127);
}

TEST(AirQualityParserTest, LongIconCodeIsTruncated) {
    AirQualityData d;
    ASSERT_TRUE(
        parseInto(R"({"data": {"current": {"weather": {"ic": "10dxyz"}, "pollution": {}}}})", d));
    EXPECT_STREQ(d.icon_code, "10d");
}

TEST(AirQualityParserTest, MissingPollutionFailsAndLeavesDataUntouched) {
    AirQualityData d;
    d.aqi_us = 123;
    EXPECT_FALSE(parseInto(R"({"data": {"current": {"weather": {"tp": 5}}}})", d));
    EXPECT_EQ(d.aqi_us, 123);
    EXPECT_EQ(d.temperature, 0);
}

TEST(AirQualityParserTest, MissingDataFails) {
    AirQualityData d;
    EXPECT_FALSE(parseInto(R"({"status": "fail"})", d));
}
