#include "ActionHandler.h"
#include <ESP.h>  // For ESP.restart()

ActionHandler::ActionHandler(ScreenManager* screenManager, ILogger& logger, LGFX* lcd)
    : screenManager(screenManager), logger_(logger), lcd_(lcd) {
    registerHandlers();
}

void ActionHandler::registerHandlers() {
    auto& eventBus = EventBus::getInstance();

    eventBus.subscribe("reset_device", [this]() { resetDevice(); });
    eventBus.subscribe("cycle_brightness", [this]() { cycleBrightness(); });
}

void ActionHandler::resetDevice() {
    logger_.info("ActionHandler::resetDevice() called");
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