#include "core/Application.h"
#include "utils/Logger.h"

Application app;

void setup() {
    delay(10);  // Brief delay for stability

    static_assert(__cplusplus >= 201703L, "Not using C++17 or higher");

    if (!app.initialize()) {
        while (true) {
            Serial.println("Init failed!");
            while (1);
        }
    }
}

void loop() {
    app.run();
    delay(10);
}