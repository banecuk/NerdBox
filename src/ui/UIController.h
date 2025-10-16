#pragma once

#include <memory>

#include "DisplayContext.h"
#include "DisplayManager.h"
#include "core/state/SystemState.h"
#include "services/PcMetrics.h"
#include "ui/screens/ScreenInterface.h"
#include "ui/screens/ScreenTypes.h"
#include "utils/ApplicationMetrics.h"
#include "utils/Logger.h"
#include "config/AppConfigInterface.h"

// Forward declarations
class BootScreen;
class MainScreen;
class SettingsScreen;
class EventHandler;

class UIController {
   public:
    explicit UIController(DisplayContext& context, DisplayManager* displayManager,
                          ApplicationMetrics& systemMetrics, PcMetrics& pcMetrics,
                          SystemState::ScreenState& screenState, AppConfigInterface& config);
    ~UIController();

    void initialize();
    bool requestsScreenTransition(ScreenName screenName);
    void updateDisplay();
    bool isTransitioning() const { return transition_.isActive; }

    DisplayContext& getDisplayContext() { return context_; }

    DisplayManager* getDisplayManager() const { return displayManager_; }

    void requestScreen(ScreenName screenName) {
        logger_.debugf("[UIController] Requesting screen %d",
                       static_cast<int>(screenName));
        requestsScreenTransition(screenName);
    }

    bool tryAcquireDisplayLock() {
        const TickType_t timeout = pdMS_TO_TICKS(200);
        BaseType_t res = xSemaphoreTake(displayMutex_, timeout);
        if (res != pdTRUE) {
            logger_.error("[UIController] Display lock timeout after 200ms");
            return false;
        }
        return true;
    }

    void releaseDisplayLock() { xSemaphoreGive(displayMutex_); }

   private:
    enum class ScreenTransitionState {
        IDLE,       // No transition in progress
        UNLOADING,  // Unloading current screen
        CLEARING,   // Clearing display
        ACTIVATING  // Loading and activating new screen
    };

    struct ScreenTransition {
        ScreenName nextScreen = ScreenName::UNSET;
        ScreenTransitionState state = ScreenTransitionState::IDLE;
        bool isActive = false;
        unsigned long startTime = 0;
    };

    void handleScreenTransition();
    void unloadCurrentScreen();
    void clearDisplay();
    void activateNextScreen();
    void resetTransitionState();

    void processTouchInput();

    LoggerInterface& logger_;
    DisplayManager* displayManager_;
    DisplayContext& context_;
    ApplicationMetrics& systemMetrics_;
    PcMetrics& pcMetrics_;
    SystemState::ScreenState& screenState_;
    AppConfigInterface& config_;

    std::unique_ptr<ScreenInterface> currentScreen_;
    std::unique_ptr<EventHandler> actionHandler_;
    SemaphoreHandle_t displayMutex_;

    ScreenTransition transition_;
};