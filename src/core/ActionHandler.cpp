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

    // eventBus.subscribe(ActionType::SHOW_ABOUT, [this]() {
    //     showAbout();
    // });
}

void ActionHandler::resetDevice() { ESP.restart(); }

void ActionHandler::cycleBrightness() { displayDriver_->cycleBrightness(); }

void ActionHandler::showSettings() { 
    logger_.info("Show Settings Screen"); 
    uiController_->setScreen(ScreenName::SETTINGS);
}