#pragma once

#include "AppConfigInterface.h"
#include "config/AppConfig.h"

class AppConfigService : public AppConfigInterface {
public:

    // Timing getters

    uint32_t getTimingScreenTaskMs() const override {
        return Config::Timing::kScreenTaskMs;
    }

    uint32_t getTimingBackgroundTaskMs() const override {
        return Config::Timing::kBackgroundTaskMs;
    }

    uint32_t getTimingMainLoopMs() const override {
        return Config::Timing::kMainLoopMs;
    }

    // Tasks getters

    uint32_t getTasksScreenStack() const override {
        return Config::Tasks::kScreenStack;
    }

    uint32_t getTasksBackgroundStack() const override {
        return Config::Tasks::kBackgroundStack;
    }

    uint32_t getTasksScreenPriority() const override {
        return Config::Tasks::kScreenPriority;
    }

    uint32_t getTasksBackgroundPriority() const override {
        return Config::Tasks::kBackgroundPriority;
    }

    // PcMetrics getters

    uint8_t getPcMetricsCores() const override {
        return Config::PcMetrics::kCores;
    }
    
};