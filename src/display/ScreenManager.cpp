#include "display/ScreenManager.h"
#include "display/screens/BootScreen.h"
#include "display/screens/MainScreen.h"
#include "core/ActionHandler.h"

ScreenManager::ScreenManager(ILogger& logger, DisplayManager* displayManager,
                           PcMetrics& pcMetrics, SystemState::ScreenState& screenState)
    : logger_(logger),
      displayManager_(displayManager),
      pcMetrics_(pcMetrics),
      screenState_(screenState),
      bootScreen_(new BootScreen(logger, displayManager->getDisplay())),
      mainScreen_(new MainScreen(logger, displayManager->getDisplay(), pcMetrics, this)),
      actionHandler_(new ActionHandler(this, logger_, displayManager_)) {

    if (!displayManager_) {
        throw std::invalid_argument("DisplayManager pointer cannot be null");
    }
}

ScreenManager::~ScreenManager() {
    // Smart pointers will automatically clean up
}

void ScreenManager::initialize() {
    setScreen(ScreenName::BOOT);
}

bool ScreenManager::setScreen(ScreenName screenName) {
    if (screenName == screenState_.activeScreen) {
        logger_.warning("Screen already active: %d", static_cast<int>(screenName));
        return false;
    }

    if (currentScreen_) {
        currentScreen_->onExit();
        clearDisplay();
    }

    switch (screenName) {
        case ScreenName::BOOT:
            currentScreen_.reset(bootScreen_.release());
            break;
        case ScreenName::MAIN:
            currentScreen_.reset(mainScreen_.release());
            break;
        case ScreenName::UNSET:
            logger_.error("Attempted to set UNSET screen");
            return false;
    }

    if (!currentScreen_) {
        logger_.error("Failed to set screen: invalid state");
        return false;
    }

    screenState_.activeScreen = screenName;
    currentScreen_->onEnter();
    return true;
}

void ScreenManager::draw() {
    if (currentScreen_) {
        currentScreen_->draw();
    }
}

void ScreenManager::handleTouchInput() {
    if (!currentScreen_ || !displayManager_) {
        return;
    }

    LGFX* lcd = displayManager_->getDisplay();
    if (!lcd) return;

    static unsigned long lastTouchTime = 0;
    constexpr unsigned long kDebounceMs = 50;

    int32_t x = 0, y = 0;
    if (lcd->getTouch(&x, &y)) {
        unsigned long currentTime = millis();
        if (currentTime - lastTouchTime > kDebounceMs) {
            lcd->fillRect(x - 2, y - 2, 4, 4, TFT_RED);
            currentScreen_->handleTouch(x, y);
            lastTouchTime = currentTime;
        }
    }
}

void ScreenManager::clearDisplay() {
    if (displayManager_ && displayManager_->getDisplay()) {
        displayManager_->getDisplay()->fillScreen(TFT_BLACK);
    }
}