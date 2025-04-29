#include "BootScreen.h"

BootScreen::BootScreen(ILogger &logger, LGFX *lcd) : logger_(logger), lcd_(lcd) {
    if (lcd == nullptr) {
        // Handle null pointer case (e.g., log an error or throw an exception)
    }
}

void BootScreen::initialize() {
    // Initialization code if needed
}

void BootScreen::onEnter() {
    lcd_->setTextSize(3);

    lcd_->setTextColor(TFT_DARKGRAY);
    lcd_->drawString("NerdBox", 1, 1);
    lcd_->setTextColor(TFT_DARKCYAN);
    lcd_->drawString("NerdBox", 0, 0);
    lcd_->setTextSize(1);
    lcd_->setTextColor(TFT_WHITE, TFT_BLACK);
    lineNumber_ = 2;
}

void BootScreen::onExit() {}

void BootScreen::draw() {
    std::queue<String> screenMessages = logger_.getScreenMessages();
    while (!screenMessages.empty()) {
        String msg = screenMessages.front();
        lcd_->setCursor(0, lineNumber_ * 18);
        lcd_->println(msg);
        screenMessages.pop();
        lineNumber_++;
    }
}