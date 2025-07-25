#include "config/AppConfig.h"
#include "core/Application.h"

Application app;

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

    // Initialize serial communication
    Serial.begin(Config::Debug::kSerialBaudRate);
    if (Config::Debug::kWaitForSerial) {
        waitForSerial(Config::Debug::kSerialTimeoutMs);
    }

    Serial.printf("Total PSRAM: %d bytes\n", ESP.getPsramSize());
    Serial.printf("Free PSRAM: %d bytes\n", ESP.getFreePsram());

    // Print the last reset reason
    esp_reset_reason_t reason = esp_reset_reason();
    if (reason != ESP_RST_POWERON) {
        Serial.printf("Last reset reason: %d\n", reason);
        esp_err_t err = esp_task_wdt_status(NULL);
        Serial.printf("WDT Status: %d\n", err);
        // Print panic details if available
        Serial.println("Panic details (if any):");
        esp_reset_reason_t reason = esp_reset_reason();
        if (reason == ESP_RST_PANIC) {
            // You may need to include esp_debug_helpers.h for more details
            Serial.println("Panic occurred. Check backtrace in debugger.");
        }
    }

    if (!app.initialize()) {
        while (true) {
            Serial.println("Init failed!");
            delay(1000);
        }
    }
}

void loop() {
    app.run();
}