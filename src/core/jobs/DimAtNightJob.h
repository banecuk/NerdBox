#pragma once

#include "config/AppSettings.h"
#include "core/BackgroundJob.h"
#include "services/NtpService.h"
#include "ui/core/DisplayManager.h"

// Watches the local clock and tells DisplayManager whether it's currently
// inside the "dim at night" window (config_.uiDimAtNightStartHour..EndHour).
// DisplayManager itself gates on whether the feature is enabled and only
// touches the display when the night/day state actually flips. The clock only
// needs hour-granularity, so polling every kCheckIntervalMs is plenty prompt
// for catching the 20:00/06:00 boundary without checking every single tick.
class DimAtNightJob : public BackgroundJob {
 public:
    DimAtNightJob(NtpService& ntpService, DisplayManager& displayManager,
                  const AppSettings& config)
        : ntpService_(ntpService), displayManager_(displayManager), config_(config) {}

    unsigned long nextDueMs() const override { return lastCheckMs_ + kCheckIntervalMs; }

    void run() override {
        lastCheckMs_ = millis();

        if (!ntpService_.isTimeSynced())
            return;

        const int hour = ntpService_.getTime().tm_hour;
        const bool isNight = (hour >= config_.uiDimAtNightStartHour) ||
                             (hour < config_.uiDimAtNightEndHour);

        displayManager_.setNightWindowActive(isNight);
    }

 private:
    static constexpr unsigned long kCheckIntervalMs = 60000;

    NtpService& ntpService_;
    DisplayManager& displayManager_;
    const AppSettings& config_;
    unsigned long lastCheckMs_ = 0;
};
