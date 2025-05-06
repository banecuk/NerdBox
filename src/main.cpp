#include "core/system/System.h"
#include "utils/Logger.h"

System app;
bool isTimeSynced = false;     // Add this global variable
Logger logger_(isTimeSynced);  // Initialize with time sync status

// uint32_t initialHeap = 0;
// const uint32_t HEAP_CRASH_THRESHOLD = 2048;  // 2KB drop threshold

void setup() {
    delay(10);  // Brief delay for stability

    static_assert(__cplusplus >= 201703L, "Not using C++17 or higher");


    // Serial.begin(115200);
    // // Wait for serial connection to be established
    // unsigned long startTime = millis();
    // const unsigned long timeout = 5000;  // 5 second timeout

    // while (!Serial && (millis() - startTime < timeout)) {
    //     delay(100);
    // }

    // Serial.println("Serial connection established!");

    // initialHeap = ESP.getFreeHeap();

    if (!app.initialize()) {
        Serial.begin(115200);
        while (true) {
            Serial.println("Init failed!");
            while (1);
        }
    }

    Serial.println("Init complete");
}

void loop() {
    // static uint32_t lastSafeHeap = initialHeap;
    // uint32_t currentHeap = ESP.getFreeHeap();

    // // Only check for heap drops after initial setup
    // if (millis() > 15000) {  // Wait 15 seconds before enabling crash detection
    //     if (currentHeap < lastSafeHeap - HEAP_CRASH_THRESHOLD) {
    //         Serial.printf("Heap drop: %u -> %u\n", lastSafeHeap,
    //                       currentHeap);
    //         while (1) {
    //             Serial.println("Heap drop detected!");
    //             delay(5000);
    //         }
    //     }
    // }

    // lastSafeHeap = currentHeap;

    app.run();

    delay(10);
}