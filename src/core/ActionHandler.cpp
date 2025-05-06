#include "ActionHandler.h"

#include <ESP.h>

#include "ui/DisplayDriver.h"
#include "ui/UIController.h"

ActionHandler::ActionHandler(UIController* uiController, ILogger& logger,
                             DisplayDriver* displayDriver)
    : uiController_(uiController), logger_(logger), displayDriver_(displayDriver) {
    registerHandlers();
}

void ActionHandler::registerHandlers() {
    auto& eventBus = EventBus::getInstance();

    eventBus.subscribe(ActionType::NONE, [this]() {
        logger_.info("ActionHandler- ActionType::NONE called");
    });

    eventBus.subscribe(ActionType::RESET_DEVICE, [this]() { resetDevice(); });

    eventBus.subscribe(ActionType::CYCLE_BRIGHTNESS, [this]() { cycleBrightness(); });

    eventBus.subscribe(ActionType::SHOW_SETTINGS, [this]() { showSettings(); });

    eventBus.subscribe(ActionType::SHOW_MAIN, [this]() { showMain(); });
}

void ActionHandler::resetDevice() { ESP.restart(); }

void ActionHandler::cycleBrightness() { displayDriver_->cycleBrightness(); }

void ActionHandler::showSettings() { 
    logger_.debug("SHOW_SETTINGS action received");
    uiController_->setScreen(ScreenName::SETTINGS);
    logger_.debug("SHOW_SETTINGS action completed");
}

void ActionHandler::showMain() { 
    logger_.debug("SHOW_MAIN action received");
    logger_.debugf("[Heap] Pre-transition: %d", ESP.getFreeHeap());
    uiController_->setScreen(ScreenName::MAIN);
    logger_.debugf("[Heap] Post-transition: %d", ESP.getFreeHeap());
    logger_.debug("SHOW_MAIN action completed");
}