#pragma once

#include <atomic>
#include <memory>

#include "config/AppSettings.h"
#include "core/IScreenUpdater.h"
#include "core/ScreenTypes.h"
#include "core/state/SystemState.h"
#include "DisplayContext.h"
#include "DisplayManager.h"
#include "network/NetworkManager.h"
#include "services/airQuality/AirQualityData.h"
#include "services/network/NetworkStatus.h"
#include "services/pcMetrics/PcMetrics.h"
#include "services/weather/WeatherData.h"
#include "ui/core/TouchManager.h"
#include "ui/screens/ScreenInterface.h"
#include "utils/ApplicationMetrics.h"
#include "utils/Logger.h"

// Forward declarations
class BootScreen;
class MainScreen;
class SettingsScreen;
class UiEventHandler;

class UiController : public IScreenUpdater {
 public:
    explicit UiController(DisplayContext& context, DisplayManager* displayManager,
                          ApplicationMetrics& systemMetrics, PcMetrics& pcMetrics,
                          SystemState::ScreenState& screenState, const AppSettings& config,
                          NetworkManager& networkManager, const AirQualityData& airQualityData,
                          const NetworkStatus& netStatus, WeatherData& weatherData);
    ~UiController();

    // Lifecycle methods
    void initialize();
    void updateDisplay() override;
    bool isTransitioning() const { return activeTransition_.isActive; }

    // Screen transition methods. Callable from any task — requestTransitionTo()
    // only ever stores into the atomic pendingScreen_; the ScreenUpdate task is
    // the sole reader/writer of activeTransition_/currentScreen_/
    // screenState_.activeScreen, so there's no cross-task race on the
    // transition state machine (see updateDisplay()).
    bool requestTransitionTo(ScreenName screenName);
    void requestScreen(ScreenName screenName) {
        logger_.debugf("[UiController] Requesting screen %d", static_cast<int>(screenName));
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
    const AppSettings& config_;
    NetworkManager& networkManager_;
    const AirQualityData& airQualityData_;
    const NetworkStatus& netStatus_;
    WeatherData& weatherData_;

    std::unique_ptr<ScreenInterface> currentScreen_;
    std::unique_ptr<UiEventHandler> actionHandler_;
    std::unique_ptr<TouchManager> touchManager_;
    SemaphoreHandle_t displayAccessMutex_;

    // Only touched by the ScreenUpdate task.
    ScreenTransition activeTransition_;

    // Cross-task handoff: requestTransitionTo() (called from the main/loop
    // task via WebServerService, the setup task via InitializationStateMachine,
    // or the ScreenUpdate task itself via touch/EventBus handling) stores here;
    // updateDisplay() drains it at the top of each tick, on the ScreenUpdate
    // task, before touching activeTransition_.
    std::atomic<ScreenName> pendingScreen_{ScreenName::NONE};
};