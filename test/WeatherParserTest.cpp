#include <ArduinoJson.h>

#include "NullLogger.h"
#include "services/weather/WeatherParser.h"

#include <gtest/gtest.h>

// open-meteo daily forecast, trimmed to 3 days. weather_code is deliberately
// fractional: open-meteo can serialise it as 61.0, and the old `| 0` read
// turned any float node into code 0 ("clear sky") (B5).
static const char* kForecastJson = R"({
  "daily": {
    "time": [1762473600, 1762560000, 1762646400],
    "weather_code": [61.0, 3.0, 95.0],
    "temperature_2m_max": [12.34, -0.06, 21.0],
    "temperature_2m_min": [-3.46, -10.5, 8],
    "rain_sum": [4.25, 0.0, 12.0],
    "wind_speed_10m_max": [5.5, 0.04, 14.96]
  }
})";

static bool parseInto(const char* json, WeatherData& data) {
    JsonDocument doc;
    deserializeJson(doc, json);
    NullLogger logger;
    return WeatherParser::parse(doc.as<JsonObjectConst>(), data, logger);
}

TEST(WeatherParserTest, ParsesEveryDay) {
    WeatherData data;
    ASSERT_TRUE(parseInto(kForecastJson, data));
    ASSERT_EQ(data.dayCount, 3);

    EXPECT_EQ(data.days[0].dayStart, 1762473600);
    EXPECT_EQ(data.days[0].tempMaxX10, 123);
    EXPECT_EQ(data.days[0].tempMinX10, -35);  // half away from zero
    EXPECT_EQ(data.days[0].rainX10, 43);
    EXPECT_EQ(data.days[0].windMaxX10, 55);

    EXPECT_EQ(data.days[1].tempMaxX10, -1);
    EXPECT_EQ(data.days[1].tempMinX10, -105);
    EXPECT_EQ(data.days[1].windMaxX10, 0);

    EXPECT_EQ(data.days[2].tempMinX10, 80);  // integer node still works
    EXPECT_EQ(data.days[2].windMaxX10, 150);
}

TEST(WeatherParserTest, FractionalWeatherCodeIsReadNotZeroed) {
    WeatherData data;
    ASSERT_TRUE(parseInto(kForecastJson, data));
    EXPECT_EQ(data.days[0].weatherCode, 61);
    EXPECT_EQ(data.days[1].weatherCode, 3);
    EXPECT_EQ(data.days[2].weatherCode, 95);
}

TEST(WeatherParserTest, IntegerWeatherCodeStillWorks) {
    WeatherData data;
    ASSERT_TRUE(parseInto(R"({"daily": {"time": [1], "weather_code": [80]}})", data));
    EXPECT_EQ(data.days[0].weatherCode, 80);
}

TEST(WeatherParserTest, MissingSeriesReadAsZero) {
    WeatherData data;
    ASSERT_TRUE(parseInto(R"({"daily": {"time": [1, 2], "weather_code": [2]}})", data));
    EXPECT_EQ(data.dayCount, 2);
    EXPECT_EQ(data.days[1].weatherCode, 0);  // array shorter than time[]
    EXPECT_EQ(data.days[0].tempMaxX10, 0);   // array absent
}

TEST(WeatherParserTest, ClampsToForecastCapacity) {
    WeatherData data;
    ASSERT_TRUE(parseInto(R"({"daily": {"time": [1,2,3,4,5,6,7,8,9,10]}})", data));
    EXPECT_EQ(data.dayCount, AppConfig::Limits::kForecastDays);
}

TEST(WeatherParserTest, MissingDailyFailsAndLeavesDataUntouched) {
    WeatherData data;
    data.dayCount = 5;
    EXPECT_FALSE(parseInto(R"({"hourly": {}})", data));
    EXPECT_EQ(data.dayCount, 5);
}

TEST(WeatherParserTest, EmptyTimeArrayFails) {
    WeatherData data;
    EXPECT_FALSE(parseInto(R"({"daily": {"time": []}})", data));
}
