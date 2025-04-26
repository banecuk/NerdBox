#include "core/system/System.h"

System app;

void setup() {
    delay(10);  // Brief delay for stability

    if (!app.initialize()) {
        Serial.begin(115200);
        while (true) {
            Serial.println("System initialization failed!");
            delay(1000);
        }  // Halt on failure
    }
}

void loop() { app.run(); }