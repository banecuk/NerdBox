#include "core/system/System.h"
#include "utils/Logger.h"

System app;

void waitForSerial(uint32_t timeoutMs) {
    uint32_t start = millis();
    while (!Serial && millis() - start < timeoutMs) {
        delay(100);
    }
    Serial.println("------------------------------");
    Serial.println("Serial connection established!");
}

void setup() {
    delay(10);  // Brief delay for stability

    static_assert(__cplusplus >= 201703L, "Not using C++17 or higher");

    Serial.begin(Config::Debug::kSerialBaudRate);
    waitForSerial(Config::Debug::kSerialTimeoutMs);

    // Print the last reset reason
    esp_reset_reason_t reason = esp_reset_reason();  // ESP_RST_WDT, ESP_RST_INT_WDT
    if (reason != ESP_RST_POWERON) {
        Serial.printf("Last reset reason: %d\n", reason);
        // Log to permanent storage if possible
    }
    Serial.println("------------------------------");

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