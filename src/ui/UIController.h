#pragma once

#include <memory>

#include "config/AppConfigInterface.h"
#include "core/state/SystemState.h"
#include "DisplayContext.h"
#include "DisplayManager.h"
#include "services/PcMetrics.h"
#include "ui/screens/ScreenInterface.h"
#include "ui/screens/ScreenTypes.h"
#include "utils/ApplicationMetrics.h"
#include "utils/Logger.h"

// Forward declarations
class BootScreen;
class MainScreen;
class SettingsScreen;
class EventHandler;

class UiController {
 public:
    explicit UiController(DisplayContext& context, DisplayManager* displayManager,
                          ApplicationMetrics& systemMetrics, PcMetrics& pcMetrics,
                          SystemState::ScreenState& screenState, AppConfigInterface& config);
    ~UiController();

    // Lifecycle methods
    void initialize();
    void updateDisplay();
    bool isTransitioning() const { return activeTransition_.isActive; }

    // Screen transition methods
    bool requestTransitionTo(ScreenName screenName);
    void requestScreen(ScreenName screenName) {
        logger_.debugf("[UIController] Requesting screen %d", static_cast<int>(screenName));
        requestTransitionTo(screenName);
    }

    // Display access methods
    DisplayContext& getDisplayContext() { return context_; }
    DisplayManager* getDisplayManager() const { return displayManager_; }
    bool tryAcquireDisplayLock();
    void releaseDisplayLock();

 private:
    enum class TransitionPhase {
        IDLE,       // No transition in progress
        UNLOADING,  // Unloading current screen
        CLEARING,   // Clearing display
        ACTIVATING  // Loading and activating new screen
    };

    struct ScreenTransition {
        ScreenName nextScreen = ScreenName::NONE;
        TransitionPhase phase = TransitionPhase::IDLE;
        bool isActive = false;
        unsigned long startTime = 0;
    };

    // Transition lifecycle methods
    void processTransitionPhase();
    void unloadCurrentScreen();
    void clearDisplay();
    void loadAndActivateScreen();
    void completeTransition();

    // Touch input methods
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
    SemaphoreHandle_t displayAccessMutex_;

    ScreenTransition activeTransition_;
};