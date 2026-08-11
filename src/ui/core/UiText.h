#pragma once

// Shared display strings for widget states, so every widget shows the same
// text instead of each one inventing its own casing ("NO DATA" vs "No Data").
namespace UiText {

// Shown centered in a widget's content area whenever its backing data is
// unavailable or stale (PcMetrics, AirQualityData, WeatherData all share the
// same DataFreshnessGuard-driven notion of "fresh").
constexpr const char* kNoData = "No Data";

}  // namespace UiText
