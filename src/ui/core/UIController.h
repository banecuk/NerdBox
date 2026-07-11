#pragma once

#include <memory>

#include "config/AppSettings.h"
#include "core/IScreenUpdater.h"
#include "core/state/SystemState.h"
#include "DisplayContext.h"
#include "DisplayManager.h"
#include "network/NetworkManager.h"
#include "services/pcMetrics/PcMetrics.h"
#include "services/airQuality/AirQualityData.h"
#include "services/network/NetworkStatus.h"
#include "ui/core/TouchManager.h"
#include "ui/screens/ScreenInterface.h"
#include "core/ScreenTypes.h"
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
                          NetworkManager& networkManager,
                          const AirQualityData& airQualityData,
                          const NetworkStatus& netStatus);
    ~UiController();

    // Lifecycle methods
    void initialize();
    void updateDisplay() override;
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
    const AppSettings& config_;
    NetworkManager& networkManager_;
    const AirQualityData& airQualityData_;
    const NetworkStatus&  netStatus_;

    std::unique_ptr<ScreenInterface> currentScreen_;
    std::unique_ptr<UiEventHandler> actionHandler_;
    std::unique_ptr<TouchManager> touchManager_;
    SemaphoreHandle_t displayAccessMutex_;

    ScreenTransition activeTransition_;
};