#pragma once

// Minimal stand-in for FreeRTOS's semaphore API, present only on
// PlatformIO's `native` platform build path (`[env:native]`). Single-
// threaded host tests never contend on these, so every operation is a no-op
// — this exists purely so ScopedLock.h/PcMetrics.h compile without pulling
// in a real FreeRTOS/ESP-IDF toolchain.

using SemaphoreHandle_t = void*;

constexpr long portMAX_DELAY = -1;

inline SemaphoreHandle_t xSemaphoreCreateMutex() { return nullptr; }
inline void vSemaphoreDelete(SemaphoreHandle_t) {}
inline int xSemaphoreTake(SemaphoreHandle_t, long) { return 1; }
inline int xSemaphoreGive(SemaphoreHandle_t) { return 1; }
