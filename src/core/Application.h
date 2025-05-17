#pragma once

#include <WebServer.h>
#include <esp_task_wdt.h>

#include <LovyanGFX.hpp>

#include "config/AppConfig.h"
#include "core/TaskManager.h"
#include "core/state/SystemState.h"
#include "network/HttpClient.h"
#include "network/NetworkManager.h"
#include "services/HttpServer.h"
#include "services/NtpService.h"
#include "services/PcMetricsService.h"
#include "ui/DisplayManager.h"
#include "ui/UIController.h"
#include "ui/screens/ScreenTypes.h"
#include "utils/Logger.h"
#include "utils/ApplicationMetrics.h"

class Application {
   public:
    enum class InitState {
        INITIAL,
        DISPLAY_INIT,
        TASKS_INIT,
        NETWORK_INIT,
        TIME_INIT,
        WATCHDOG_INIT,
        FINAL_SETUP,
        COMPLETE,
        FAILED
    };

    Application();
    ~Application() = default;

    // Delete copy/move operations
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&) = delete;
    Application& operator=(Application&&) = delete;

    bool initialize();
    void run();

   private:
    // Initialization Methods
    bool initializeDisplay();
    bool initializeNetwork(uint8_t maxRetries);
    bool initializeTimeService(uint8_t maxRetries);
    bool initializeWatchdog();
    void completeInitialization();

    // State Management
    void transitionTo(InitState newState);
    bool handleStateTransition();
    bool isTerminalState() const;

    // Logging Helpers
    void logCompletionStatus(bool success) const;
    String getStateName(InitState state) const;
    void logRetryAttempt(const char* component, uint8_t attempt,
                         uint8_t maxRetries) const;
    uint16_t calculateBackoffDelay(uint8_t attempt, uint16_t baseDelay) const;
    void handleInitializationFailure();

    // Application Components
    LGFX display_;
    DisplayManager displayManager_;
    WebServer webServer_;

    // Managers and Services
    SystemState systemState_;
    Logger logger_;
    UIController uiController_;
    NetworkManager networkManager_;
    HttpClient httpClient_;
    NtpService ntpService_;
    HttpServer httpServer_;
    PcMetricsService pcMetricsService_;
    TaskManager taskManager_;
    ApplicationMetrics systemMetrics_;

    // Initialization State
    InitState currentInitState_;

    static constexpr const char* INIT_STATE_NAMES_[] = {
        "INITIAL",       "DISPLAY_INIT", "TASKS_INIT", "NETWORK_INIT", "TIME_INIT",
        "WATCHDOG_INIT", "FINAL_SETUP",  "COMPLETE",   "FAILED"};
};