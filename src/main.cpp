#include <memory>

#include "core/Application.h"
#include "core/ApplicationComponents.h"

static AppSettings config;
static std::unique_ptr<Application> app;

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
    Serial.begin(config.debugSerialBaudRate);

    if (config.debugWaitForSerial) {
        waitForSerial(config.debugSerialTimeoutMs);
    }

    Serial.printf("Total PSRAM: %u bytes\n", ESP.getPsramSize());
    Serial.printf("Free PSRAM: %u bytes\n", ESP.getFreePsram());

    // Print the last reset reason
    esp_reset_reason_t reason = esp_reset_reason();
    if (reason != ESP_RST_POWERON) {
        Serial.printf("Last reset reason: %d\n", reason);
        esp_err_t err = esp_task_wdt_status(NULL);
        Serial.printf("WDT Status: %d\n", err);
        // Print panic details if available
        Serial.println("Panic details (if any):");
        if (reason == ESP_RST_PANIC) {
            Serial.println("Panic occurred. Check backtrace in debugger.");
        }
    }

    // Create application instance
    app = std::make_unique<Application>(std::make_unique<ApplicationComponents>());

    if (!app->initialize()) {
        app.reset();
        Serial.println("Init failed! Rebooting in 5s...");
        delay(5000);
        esp_restart();
    }
}

void loop() {
    if (app) {
        app->run();
    } else {
        // Safety fallback
        delay(1000);
    }
}