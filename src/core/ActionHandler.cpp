#include "ActionHandler.h"
#include <ESP.h>

ActionHandler::ActionHandler(ScreenManager* screenManager, ILogger& logger, LGFX* lcd)
    : screenManager(screenManager), logger_(logger), lcd_(lcd) {
    registerHandlers();
}

void ActionHandler::registerHandlers() {
    auto& eventBus = EventBus::getInstance();

    eventBus.subscribe(ActionType::NONE, [this]() {
        logger_.info("ActionHandler- ActionType::NONE called");
    });
    

    eventBus.subscribe(ActionType::RESET_DEVICE, [this]() {
        resetDevice();
    });
    
    eventBus.subscribe(ActionType::CYCLE_BRIGHTNESS, [this]() {
        cycleBrightness();
    });
    
    // eventBus.subscribe(ActionType::SHOW_SETTINGS, [this]() {
    //     showSettings();
    // });
    
    // eventBus.subscribe(ActionType::SHOW_ABOUT, [this]() {
    //     showAbout();
    // });
}

void ActionHandler::resetDevice() {
    ESP.restart();
}

void ActionHandler::cycleBrightness() {
    logger_.info("ActionHandler::cycleBrightness() called");

    static uint8_t brightnessLevel = 0;
    brightnessLevel = (brightnessLevel + 1) % 3;  // Cycle through 3 levels

    uint8_t brightness;
    switch (brightnessLevel) {
        case 0:
            brightness = 25;
            break;
        case 1:
            brightness = 100;
            break;
        case 2:
            brightness = 255;
            break;
    }

    lcd_->setBrightness(brightness);
}