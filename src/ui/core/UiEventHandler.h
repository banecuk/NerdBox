#pragma once

#include <array>

#include "core/events/EventBus.h"
#include "utils/logging/Logger.h"

// Forward declarations
class UiController;
class DisplayManager;

// UiEventHandler — translates EventBus events into UiController / DisplayManager actions.
//
// Subscription table (registered in registerHandlers()):
//   EventType::NONE            → log only (debug aid)
//   EventType::RESET_DEVICE    → resetDevice()      — shows message, calls ESP.restart()
//   EventType::CYCLE_BRIGHTNESS → cycleBrightness() — steps through brightness levels & saves to
//   NVS EventType::SHOW_SETTINGS   → requestSettingsScreen() EventType::SHOW_MAIN       →
//   requestMainScreen() EventType::SHOW_GAME       → requestGameScreen() EventType::SHOW_DISKS →
//   requestDisksScreen() EventType::SHOW_WEATHER    → requestWeatherScreen()
//   EventType::SHOW_CALENDAR  → requestCalendarScreen()
class UiEventHandler {
 public:
    UiEventHandler(UiController* uiController, LoggerInterface& logger);
    ~UiEventHandler();

    void registerHandlers();
    void resetDevice();
    void cycleBrightness();
    void requestSettingsScreen();
    void requestMainScreen();
    void requestGameScreen();
    void requestDisksScreen();
    void requestCpuClockScreen();
    void requestWeatherScreen();
    void requestCalendarScreen();

 private:
    UiController* uiController_;
    LoggerInterface& logger_;

    struct Subscription {
        EventType type;
        EventBus::SubscriptionId id;
    };
    static constexpr size_t kSubscriptionCount = 10;
    std::array<Subscription, kSubscriptionCount> subscriptions_{};
};
