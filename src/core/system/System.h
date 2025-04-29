#ifndef SYSTEM_H
#define SYSTEM_H

#include <WebServer.h>
#include <esp_task_wdt.h>
#include <LovyanGFX.hpp>

#include "config/AppConfig.h"
#include "core/TaskManager.h"
#include "core/system/SystemState.h"
#include "network/NetworkManager.h"
#include "services/HttpServer.h"
#include "services/NtpService.h"
#include "services/PcMetricsService.h"
#include "ui/ScreenTypes.h"
#include "ui/UIController.h"
#include "utils/Logger.h"

class System {
   public:
    enum class InitState {
        INITIAL,
        SERIAL_INIT,
        DISPLAY_INIT,
        TASKS_INIT,
        NETWORK_INIT,
        TIME_INIT,
        WATCHDOG_INIT,
        FINAL_SETUP,
        COMPLETE,
        FAILED
    };

    System();
    ~System() = default;

    // Delete copy/move operations
    System(const System&) = delete;
    System& operator=(const System&) = delete;
    System(System&&) = delete;
    System& operator=(System&&) = delete;

    bool initialize();
    void run();

   private:
    // Initialization Methods
    bool initializeAll();
    bool initializeSerial();
    bool initializeDisplay();
    bool initializeNetwork(uint8_t maxRetries);
    bool initializeTimeService(uint8_t maxRetries);
    bool initializeWatchdog();
    void performFinalSetup();

    // State Management
    void transitionTo(InitState newState);
    bool handleStateTransition();
    bool isTerminalState() const;

    // Logging Helpers
    void logCompletionStatus(bool success) const;
    String getStateName(InitState state) const;
    void logRetryAttempt(const char* component, uint8_t attempt, uint8_t maxRetries) const;
    uint16_t calculateBackoffDelay(uint8_t attempt, uint16_t baseDelay) const;
    void handleInitializationFailure();

    // System Components
    LGFX display_;
    DisplayDriver displayDriver_;
    WebServer webServer_;

    // Managers and Services
    SystemState systemState_;
    Logger logger_;
    UIController uiController_;
    NetworkManager networkManager_;
    HttpDownloader httpDownloader_;
    NtpService ntpService_;
    HttpServer httpServer_;
    PcMetricsService pcMetricsService_;
    TaskManager taskManager_;

    // Initialization State
    InitState currentInitState_;
    
    static constexpr const char* INIT_STATE_NAMES_[] = {
        "INITIAL",   "SERIAL_INIT",   "DISPLAY_INIT", "TASKS_INIT", "NETWORK_INIT",
        "TIME_INIT", "WATCHDOG_INIT", "FINAL_SETUP",  "COMPLETE",   "FAILED"};
};
#endif  // SYSTEM_H