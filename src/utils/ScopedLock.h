#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

// Generic RAII mutex guard: takes `mutex` on construction, gives it back on
// destruction. Replaces PcMetricsDiskLock/WeatherDataLock, two hand-written,
// byte-identical guards that existed only because each owning struct wrote
// its own copy of this pattern.
//
// Usage:
//   { ScopedLock lock(metrics.disk_drivesMutex); use metrics.disk_drives; }
class ScopedLock {
 public:
    explicit ScopedLock(SemaphoreHandle_t mutex) : mutex_(mutex) {
        xSemaphoreTake(mutex_, portMAX_DELAY);
    }
    ~ScopedLock() { xSemaphoreGive(mutex_); }

    ScopedLock(const ScopedLock&) = delete;
    ScopedLock& operator=(const ScopedLock&) = delete;

 private:
    SemaphoreHandle_t mutex_;
};
