#include "System.h"

#include <esp_task_wdt.h>

// Constructor ----------------------------------------------------------------

System::System()
    : webServer_(80),
      logger_(systemState_.core.isTimeSynced),
      screenManager_(logger_, &displayManager_, systemState_.hmData,
                     systemState_.screen),
      networkManager_(logger_),
      displayManager_(display_, logger_),
      taskManager_(logger_, screenManager_, hmDataService_, systemState_.hmData,
                   systemState_.core, systemState_.screen) {}

// Public Methods -------------------------------------------------------------

bool System::initialize() {
    SystemInit initializer(logger_, systemState_.core, display_, displayManager_,
                           screenManager_, networkManager_, ntpService_, taskManager_,
                           httpServer_, systemState_.screen);

    if (!initializer.initializeAll()) {
        handleInitializationFailure();
        return false;
    }

    screenManager_.setScreen(ScreenName::MAIN);
    systemState_.core.isInitialized = true;
    return true;
}

void System::run() {
    if (!systemState_.core.isInitialized) {
        return;
    }

    if (Config::Watchdog::kEnableOnBoot) {
        esp_task_wdt_reset();
    }

    httpServer_.processRequests();
    vTaskDelay(Config::Timing::kMainLoopMs);
}

// Private Methods ------------------------------------------------------------

void System::handleInitializationFailure() {
    logger_.critical("[FATAL] System initialization failed");
}